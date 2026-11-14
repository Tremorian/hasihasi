// HasiHasi Web Client – WebSocket Client
// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

class WebSocketClient {
  constructor() {
    this.ws = null;
    this.url = '';
    this.isConnected = false;
    this.isConnecting = false;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 1000;
    
    // Callbacks
    this.onConnected = null;
    this.onDisconnected = null;
    this.onMessage = null;
    this.onError = null;
  }
  
  async connect(url, username, password = '') {
    if (this.isConnecting || this.isConnected) {
      return false;
    }
    
    this.url = url;
    this.isConnecting = true;
    
    try {
      const wsUrl = url.startsWith('wss://') ? url : `wss://${url}`;
      this.ws = new WebSocket(wsUrl);
      
      this.ws.onopen = () => {
        console.log('WebSocket: Verbunden');
        this.isConnected = true;
        this.isConnecting = false;
        this.reconnectAttempts = 0;
        
        // Authentifizierung
        this.send({
          type: 'auth',
          username: username,
          password: password
        });
        
        if (this.onConnected) {
          this.onConnected();
        }
      };
      
      this.ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data);
          if (this.onMessage) {
            this.onMessage(message);
          }
        } catch (error) {
          console.error('WebSocket: Fehler beim Verarbeiten der Nachricht:', error);
        }
      };
      
      this.ws.onclose = (event) => {
        console.log('WebSocket: Getrennt:', event.code);
        this.isConnected = false;
        this.isConnecting = false;
        
        if (this.onDisconnected) {
          this.onDisconnected(event.code);
        }
        
        // Automatisch wieder verbinden
        if (event.code !== 1000 && this.reconnectAttempts < this.maxReconnectAttempts) {
          this.reconnect();
        }
      };
      
      this.ws.onerror = (error) => {
        console.error('WebSocket: Fehler:', error);
        if (this.onError) {
          this.onError(error);
        }
      };
      
      return true;
    } catch (error) {
      console.error('WebSocket: Verbindungsfehler:', error);
      this.isConnecting = false;
      return false;
    }
  }
  
  disconnect() {
    if (this.ws) {
      this.ws.close(1000, 'Client trennt sich');
    }
    this.isConnected = false;
    this.isConnecting = false;
  }
  
  send(message) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      console.warn('WebSocket: Nicht verbunden, Nachricht kann nicht gesendet werden');
      return false;
    }
    
    try {
      this.ws.send(JSON.stringify(message));
      return true;
    } catch (error) {
      console.error('WebSocket: Fehler beim Senden:', error);
      return false;
    }
  }
  
  sendVoiceData(data) {
    // Voice-Daten als Binary senden
    if (!this.ws || this.ws.readyState !== WebSocket.BINARY) {
      console.warn('WebSocket: Binär-WebSocket nicht verfügbar');
      return false;
    }
    
    this.ws.send(data);
  }
  
  reconnect() {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.log('WebSocket: Maximale Wiederverbindungsversuche erreicht');
      return;
    }
    
    this.reconnectAttempts++;
    const delay = this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1);
    
    console.log(`WebSocket: Wiederverbindungsversuch ${this.reconnectAttempts}/${this.maxReconnectAttempts} in ${delay}ms`);
    
    setTimeout(() => {
      this.connect(this.url);
    }, delay);
  }
  
  getLatency() {
    if (!this.ws) return 0;
    return this.ws.latency || 0;
  }
}
