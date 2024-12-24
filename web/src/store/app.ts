/* eslint-disable import/prefer-default-export */
import TemperatureScale from "@/enums/TemperatureScale";
import WebConn from "@/helpers/webConn";
import type { IMashSchedule } from "@/interfaces/IMashSchedule";
import { defineStore } from "pinia";
import { ref } from "vue";

export const useAppStore = () => {
  const innerStore = defineStore("app", () => {
    const systemSettingsLoaded = ref(false);
    const temperatureScale = ref<TemperatureScale>(TemperatureScale.Celsius);
    const tempUnit = ref("°C");
    const rootUrl = ref<string | null>(null);
    const mashSchedules = ref<Array<IMashSchedule>>([]);
    const maxSchedules = ref<number>(10); // atm this is fixed to 10

    async function getMashSchedules() {
      if (rootUrl.value == null) {
        return;
      }
      // we only need to get the mashschedules once
      const requestData = {
        command: "GetMashSchedules",
        data: null,
      };
      const webConn = new WebConn(rootUrl.value);
      const apiResult = await webConn?.doPostRequest(requestData);

      if (apiResult === undefined || apiResult.success === false) {
        return;
      }

      mashSchedules.value = apiResult.data;
    }

    async function getSystemSettings() {
      if (rootUrl.value == null) {
        return;
      }

      const webConn = new WebConn(rootUrl.value);
      const requestData = {
        command: "GetSystemSettings",
        data: null,
      };

      const apiResult = await webConn?.doPostRequest(requestData);

      if (apiResult === undefined || apiResult.success === false) {
        return;
      }

      temperatureScale.value = apiResult.data.temperatureScale;
      if (temperatureScale.value === TemperatureScale.Fahrenheit) {
        tempUnit.value = "°F";
      }

      // also get our schedules
      getMashSchedules();

      systemSettingsLoaded.value = true;
    }

    return {
      systemSettingsLoaded,
      rootUrl,
      temperatureScale,
      tempUnit,
      mashSchedules,
      maxSchedules,
      getSystemSettings,
      getMashSchedules,
    };
  });

  const s = innerStore();
  // load system settings when possible
  // if (s.rootUrl !== null && !s.systemSettingsLoaded) {
  //   s.getSystemSettings();
  // }
  return s;
};
