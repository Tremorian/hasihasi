// HasiHasi Web Client – Audio Engine
// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

class AudioEngine {
  constructor() {
    this.ctx = null;
    this.stream = null;
    this.analyser = null;
    this.gainNode = null;
    this.noiseSuppression = null;
    this.echoCancellation = null;
    this.autoGain = false;
    this.isInitialized = false;
    this.volume = 0.8;
    
    // Callbacks
    this.onVolumeChange = null;
    this.onSpeaking = null;
  }
  
  async init() {
    try {
      this.ctx = new (window.AudioContext || window.webkitAudioContext)();
      this.analyser = this.ctx.createAnalyser();
      this.analyser.fftSize = 2048;
      this.analyser.smoothingTimeConstant = 0.8;
      
      this.gainNode = this.ctx.createGain();
      this.gainNode.gain.value = this.volume;
      
      // Mikrofon-Zugriff anfordern
      this.stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: true,
          noiseSuppression: true,
          autoGainControl: false
        }
      });
      
      const source = this.ctx.createMediaStreamSource(this.stream);
      source.connect(this.analyser);
      this.analyser.connect(this.gainNode);
      
      this.isInitialized = true;
      console.log('AudioEngine: Initialisiert');
      return true;
    } catch (error) {
      console.error('AudioEngine: Fehler beim Initialisieren:', error);
      return false;
    }
  }
  
  getVolume() {
    if (!this.analyser) return 0;
    
    const data = new Uint8Array(this.analyser.frequencyBinCount);
    this.analyser.getByteFrequencyData(data);
    
    let sum = 0;
    for (let i = 0; i < data.length; i++) {
      sum += data[i];
    }
    
    return sum / data.length / 255;
  }
  
  isSpeaking(threshold = 0.1) {
    return this.getVolume() > threshold;
  }
  
  setVolume(volume) {
    this.volume = Math.max(0, Math.min(1, volume));
    if (this.gainNode) {
      this.gainNode.gain.value = this.volume;
    }
    if (this.onVolumeChange) {
      this.onVolumeChange(this.volume);
    }
  }
  
  setNoiseSuppression(enabled) {
    this.noiseSuppression = enabled;
  }
  
  setEchoCancellation(enabled) {
    this.echoCancellation = enabled;
  }
  
  setAutoGain(enabled) {
    this.autoGain = enabled;
  }
  
  startMonitoring(callback, threshold = 0.1) {
    if (!this.isInitialized) return;
    
    const monitor = () => {
      if (!this.isInitialized) return;
      
      const volume = this.getVolume();
      const speaking = volume > threshold;
      
      if (callback) {
        callback(volume, speaking);
      }
      
      requestAnimationFrame(monitor);
    };
    
    monitor();
  }
  
  stopMonitoring() {
    this.isInitialized = false;
  }
  
  async destroy() {
    if (this.stream) {
      this.stream.getTracks().forEach(track => track.stop());
    }
    if (this.ctx) {
      await this.ctx.close();
    }
    this.isInitialized = false;
  }
}
