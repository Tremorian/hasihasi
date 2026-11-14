// HasiHasi Web Client – Internationalization (i18n)
// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

class I18nManager {
  constructor() {
    this.translations = {};
    this.currentLocale = 'de';
    this.fallbackLocale = 'en';
    this.isInitialized = false;
  }
  
  async init() {
    // Locale automatisch erkennen
    const browserLocale = navigator.language || navigator.userLanguage;
    const locale = browserLocale.split('-')[0];
    
    // Unterstützte Sprachen
    const supportedLocales = ['de', 'en', 'fr', 'es'];
    if (supportedLocales.includes(locale)) {
      this.currentLocale = locale;
    }
    
    // Übersetzungen laden
    await this.loadTranslations(this.currentLocale);
    
    this.isInitialized = true;
    console.log(`I18n: Initialisiert mit Locale: ${this.currentLocale}`);
  }
  
  async loadTranslations(locale) {
    try {
      const response = await fetch(`/translations/${locale}.json`);
      if (response.ok) {
        this.translations = await response.json();
      } else {
        console.warn(`I18n: Übersetzungen für ${locale} nicht gefunden`);
        // Fallback auf Englisch
        if (locale !== this.fallbackLocale) {
          await this.loadTranslations(this.fallbackLocale);
        }
      }
    } catch (error) {
      console.error('I18n: Fehler beim Laden der Übersetzungen:', error);
    }
  }
  
  t(key, params = {}) {
    let translation = this.translations[key] || key;
    
    // Parameter ersetzen
    Object.entries(params).forEach(([param, value]) => {
      translation = translation.replace(`{${param}}`, value);
    });
    
    return translation;
  }
  
  setLocale(locale) {
    if (this.translations[locale]) {
      this.currentLocale = locale;
      this.applyTranslations();
    } else {
      console.warn(`I18n: Locale ${locale} nicht unterstützt`);
    }
  }
  
  getCurrentLocale() {
    return this.currentLocale;
  }
  
  applyTranslations() {
    // Alle Elemente mit data-i18n-Attribut übersetzen
    document.querySelectorAll('[data-i18n]').forEach(element => {
      const key = element.getAttribute('data-i18n');
      const translation = this.t(key);
      
      if (element.tagName === 'INPUT') {
        element.placeholder = translation;
      } else {
        element.textContent = translation;
      }
    });
  }
  
  getSupportedLocales() {
    return ['de', 'en', 'fr', 'es'];
  }
}
