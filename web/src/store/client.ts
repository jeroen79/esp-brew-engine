/* eslint-disable import/prefer-default-export */
import { IClientSettings } from '@/interfaces/IClientSettings';
import { defineStore } from 'pinia';
import { ref } from 'vue';

const defaultClientSettings:IClientSettings = {
  beepEnabled: true,
  beepVolume: 0.5,
  speechEnabled: false,
  speechVolume: 0.5,
  speechRate: 0.7,
  voiceUri: null,
};

export const useClientStore = () => {
  const innerStore = defineStore('client', () => {
    const clientSettingsLoaded = ref(false);
    const clientSettings = ref<IClientSettings>(defaultClientSettings);

    async function getLocalSettings() {
      const jsonSettings = localStorage.getItem('clientSettings');

      if (jsonSettings != null) {
        clientSettings.value = JSON.parse(jsonSettings);
      } else {
        clientSettings.value = defaultClientSettings;
      }

      clientSettingsLoaded.value = true;
    }

    async function saveLocalSettings() {
      const jsonSettings = JSON.stringify(clientSettings.value);

      localStorage.setItem('clientSettings', jsonSettings);
    }

    return {
      clientSettingsLoaded, getLocalSettings, saveLocalSettings, clientSettings,
    };
  });

  const s = innerStore();

  // load client settings when possible
  if (!s.clientSettingsLoaded) {
    s.getLocalSettings();
  }
  return s;
};
