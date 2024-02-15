/* eslint-disable import/prefer-default-export */
/* eslint-disable import/order */
import TemperatureScale from '@/enums/TemperatureScale';
import WebConn from '@/helpers/webConn';
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useAppStore = () => {
  const innerStore = defineStore('app', () => {
    const systemSettingsLoaded = ref(false);
    const temperatureScale = ref<TemperatureScale>(TemperatureScale.Celsius);
    const tempUnit = ref('°C');
    const rootUrl = ref<string | null>(null);

    async function getSystemSettings() {
      if (rootUrl.value == null) {
        return;
      }

      const webConn = new WebConn(rootUrl.value);
      const requestData = {
        command: 'GetSystemSettings',
        data: null,
      };

      const apiResult = await webConn?.doPostRequest(requestData);

      if (apiResult === undefined || apiResult.success === false) {
        return;
      }

      temperatureScale.value = apiResult.data.temperatureScale;
      if (temperatureScale.value === TemperatureScale.Fahrenheit) {
        tempUnit.value = '°F';
      }

      systemSettingsLoaded.value = true;
    }

    return {
      systemSettingsLoaded, rootUrl, temperatureScale, tempUnit, getSystemSettings,
    };
  });

  const s = innerStore();
  // load system settings when possible
  // if (s.rootUrl !== null && !s.systemSettingsLoaded) {
  //   s.getSystemSettings();
  // }
  return s;
};
