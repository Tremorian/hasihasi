// HasiHasi Web Client – Main Application
// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import { I18nManager } from './i18n.js';
import { AudioEngine } from './audio-engine.js';
import { WebSocketClient } from './websocket-client.js';

class HasiHasiApp {
  constructor() {
    this.i18n = new I18nManager();
    this.audio = new AudioEngine();
    this.ws = new WebSocketClient();
    
    this.currentScreen = 'splash';
    this.username = '';
    this.isPTTActive = false;
    
    this.init();
  }
  
  async init() {
    // Splash Screen
    await this.showScreen('splash');
    
    // I18n initialisieren
    await this.i18n.init();
    
    // Audio initialisieren
    const audioInitialized = await this.audio.init();
    if (!audioInitialized) {
      console.error('Audio konnte nicht initialisiert werden');
    }
    
    // UI-Events verbinden
    this.bindEvents();
    
    // Nach kurzer Verzögerung zum Login-Screen wechseln
    setTimeout(async () => {
      await this.showScreen('login');
    }, 1500);
  }
  
  bindEvents() {
    // Login-Formular
    const loginForm = document.getElementById('login-form');
    if (loginForm) {
      loginForm.addEventListener('submit', (e) => this.handleLogin(e));
    }
    
    // Theme-Toggle
    const themeToggle = document.getElementById('theme-toggle');
    if (themeToggle) {
      themeToggle.addEventListener('click', () => this.toggleTheme());
    }
    
    // PTT-Button
    const voiceBtn = document.getElementById('voice-btn');
    if (voiceBtn) {
      voiceBtn.addEventListener('mousedown', () => this.startPTT());
      voiceBtn.addEventListener('mouseup', () => this.stopPTT());
      voiceBtn.addEventListener('mouseleave', () => this.stopPTT());
      
      // Touch-Support
      voiceBtn.addEventListener('touchstart', (e) => {
        e.preventDefault();
        this.startPTT();
      });
      voiceBtn.addEventListener('touchend', (e) => {
        e.preventDefault();
        this.stopPTT();
      });
    }
    
    // Chat-Input
    const chatInput = document.getElementById('chat-input');
    const sendBtn = document.getElementById('send-btn');
    
    if (chatInput) {
      chatInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
          e.preventDefault();
          this.sendMessage();
        }
      });
    }
    
    if (sendBtn) {
      sendBtn.addEventListener('click', () => this.sendMessage());
    }
    
    // Settings
    const settingsBtn = document.getElementById('settings-menu');
    const closeSettingsBtn = document.getElementById('close-settings');
    const saveSettingsBtn = document.getElementById('save-settings');
    const cancelSettingsBtn = document.getElementById('cancel-settings');
    
    if (settingsBtn) {
      settingsBtn.addEventListener('click', () => this.openSettings());
    }
    
    if (closeSettingsBtn) {
      closeSettingsBtn.addEventListener('click', () => this.closeSettings());
    }
    
    if (saveSettingsBtn) {
      saveSettingsBtn.addEventListener('click', () => this.saveSettings());
    }
    
    if (cancelSettingsBtn) {
      cancelSettingsBtn.addEventListener('click', () => this.closeSettings());
    }
    
    // Disconnect
    const disconnectBtn = document.getElementById('disconnect-btn');
    if (disconnectBtn) {
      disconnectBtn.addEventListener('click', () => this.disconnect());
    }
    
    // Audio-Volume
    const volumeInput = document.getElementById('volume-input');
    const volumeOutput = document.getElementById('volume-output');
    
    if (volumeInput) {
      volumeInput.addEventListener('input', (e) => {
        this.audio.setVolume(e.target.value / 100);
      });
    }
    
    if (volumeOutput) {
      volumeOutput.addEventListener('input', (e) => {
        // Ausgabepegel (hier simuliert)
      });
    }
    
    // Noise Suppression
    const noiseSuppression = document.getElementById('noise-suppression');
    if (noiseSuppression) {
      noiseSuppression.addEventListener('change', (e) => {
        this.audio.setNoiseSuppression(e.target.checked);
      });
    }
    
    // Echo Cancellation
    const echoCancellation = document.getElementById('echo-cancellation');
    if (echoCancellation) {
      echoCancellation.addEventListener('change', (e) => {
        this.audio.setEchoCancellation(e.target.checked);
      });
    }
  }
  
  async showScreen(screenName) {
    // Alle Screens ausblenden
    document.querySelectorAll('.screen').forEach(screen => {
      screen.classList.add('hidden');
    });
    
    // Gewünschten Screen einblenden
    const screen = document.getElementById(`${screenName}-screen`);
    if (screen) {
      screen.classList.remove('hidden');
      this.currentScreen = screenName;
    }
    
    // Splash Screen ausblenden nach Animation
    if (screenName === 'splash') {
      const splash = document.getElementById('splash-screen');
      if (splash) {
        await new Promise(resolve => {
          setTimeout(resolve, 500);
        });
        splash.style.opacity = '0';
        setTimeout(() => {
          splash.classList.add('hidden');
        }, 500);
      }
    }
  }
  
  async handleLogin(e) {
    e.preventDefault();
    
    const username = document.getElementById('username').value;
    const serverUrl = document.getElementById('server-url').value;
    const password = document.getElementById('password').value;
    
    if (!username || !serverUrl) {
      return;
    }
    
    this.username = username;
    
    // Login-Button Loader anzeigen
    const loginBtn = document.querySelector('.btn-login');
    const btnText = loginBtn.querySelector('.btn-text');
    const btnLoader = loginBtn.querySelector('.btn-loader');
    
    btnText.classList.add('hidden');
    btnLoader.classList.remove('hidden');
    loginBtn.disabled = true;
    
    // Verbindung herstellen
    const connected = await this.ws.connect(serverUrl, username, password);
    
    if (connected) {
      // Zum Main-Screen wechseln
      await this.showScreen('main');
      
      // Audio-Monitoring starten
      this.audio.startMonitoring((volume, speaking) => {
        this.updateVoiceIndicator(speaking);
      }, 0.1);
      
      // Willkommensnachricht senden
      this.ws.send({
        type: 'chat',
        message: `🐰 ${username} hat den Kanal betreten`
      });
    } else {
      // Fehler anzeigen
      alert('Verbindung fehlgeschlagen. Bitte prüfe die Server-URL.');
    }
    
    // Login-Button zurücksetzen
    btnText.classList.remove('hidden');
    btnLoader.classList.add('hidden');
    loginBtn.disabled = false;
  }
  
  updateVoiceIndicator(speaking) {
    const voiceBtn = document.getElementById('voice-btn');
    const voiceWave = voiceBtn?.querySelector('.voice-wave');
    
    if (speaking && voiceWave) {
      voiceWave.classList.remove('hidden');
    } else if (voiceWave) {
      voiceWave.classList.add('hidden');
    }
  }
  
  startPTT() {
    this.isPTTActive = true;
    const voiceBtn = document.getElementById('voice-btn');
    if (voiceBtn) {
      voiceBtn.style.transform = 'scale(0.95)';
    }
    
    // PTT aktivieren
    if (this.audio.onSpeaking) {
      this.audio.onSpeaking(true);
    }
  }
  
  stopPTT() {
    this.isPTTActive = false;
    const voiceBtn = document.getElementById('voice-btn');
    if (voiceBtn) {
      voiceBtn.style.transform = 'scale(1)';
    }
    
    // PTT deaktivieren
    if (this.audio.onSpeaking) {
      this.audio.onSpeaking(false);
    }
  }
  
  sendMessage() {
    const chatInput = document.getElementById('chat-input');
    if (!chatInput || !chatInput.value.trim()) {
      return;
    }
    
    const message = chatInput.value.trim();
    
    // Nachricht an Server senden
    this.ws.send({
      type: 'chat',
      message: message
    });
    
    // Nachricht lokal anzeigen
    this.addChatMessage(this.username, message);
    
    // Input leeren
    chatInput.value = '';
  }
  
  addChatMessage(author, text) {
    const chatMessages = document.getElementById('chat-messages');
    if (!chatMessages) return;
    
    const messageDiv = document.createElement('div');
    messageDiv.className = 'chat-message';
    
    const initial = author.charAt(0).toUpperCase();
    const time = new Date().toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' });
    
    messageDiv.innerHTML = `
      <div class="message-avatar">${initial}</div>
      <div class="message-content">
        <div class="message-header">
          <span class="message-author">${author}</span>
          <span class="message-time">${time}</span>
        </div>
        <p class="message-text">${this.escapeHtml(text)}</p>
      </div>
    `;
    
    chatMessages.appendChild(messageDiv);
    chatMessages.scrollTop = chatMessages.scrollHeight;
  }
  
  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }
  
  toggleTheme() {
    const body = document.body;
    const currentTheme = body.classList.contains('theme-dark') ? 'dark' : 'light';
    const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
    
    body.classList.remove(`theme-${currentTheme}`);
    body.classList.add(`theme-${newTheme}`);
    
    // Theme speichern
    localStorage.setItem('hasihasi-theme', newTheme);
  }
  
  openSettings() {
    const modal = document.getElementById('settings-modal');
    if (modal) {
      modal.classList.remove('hidden');
    }
  }
  
  closeSettings() {
    const modal = document.getElementById('settings-modal');
    if (modal) {
      modal.classList.add('hidden');
    }
  }
  
  saveSettings() {
    // Einstellungen speichern
    const theme = document.getElementById('theme-select')?.value;
    if (theme) {
      document.body.classList.remove('theme-dark', 'theme-light');
      document.body.classList.add(`theme-${theme}`);
      localStorage.setItem('hasihasi-theme', theme);
    }
    
    this.closeSettings();
  }
  
  disconnect() {
    this.ws.disconnect();
    this.audio.stopMonitoring();
    this.showScreen('login');
  }
}

// App starten
document.addEventListener('DOMContentLoaded', () => {
  window.app = new HasiHasiApp();
});
