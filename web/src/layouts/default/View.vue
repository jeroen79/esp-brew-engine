<script lang="ts" setup>
import { ref } from 'vue';
import { mdiKnob, mdiAltimeter, mdiReceiptTextClock, mdiThermometerLines, mdiThermometer, mdiWifi, mdiWrenchCogOutline, mdiHeatingCoil, mdiCookieSettingsOutline, mdiFullscreen, mdiFullscreenExit } from '@mdi/js';

const drawer = ref(true);
const linksBrewing = ref([
  [mdiKnob, 'Control', 'control'],
]);

const linksTools = ref([
  [mdiAltimeter, 'Refractometer', 'refractometer'],
]);

const linksSettings = ref([
  [mdiReceiptTextClock, 'Schedules', 'mashschedules'],
  [mdiThermometerLines, 'PID Settings', 'pidsettings'],
  [mdiThermometer, 'Temperature Settings', 'tempsettings'],
  [mdiHeatingCoil, 'Heater Settings', 'heaterSettings'],
  [mdiWifi, 'Wifi Settings', 'wifiSettings'],
  [mdiWrenchCogOutline, 'System Settings', 'systemSettings'],
  [mdiCookieSettingsOutline, 'Client Settings', 'clientSettings'],
]);

const version = ref<string>(import.meta.env.VITE_APP_VERSION);

const fullscreen = ref<boolean>(false);

if (import.meta.env.MODE === 'development') {
  version.value = `${import.meta.env.VITE_APP_VERSION}_dev`;
}

const enterFullscreen = () => {
  console.log('Going fullscreen');
  const elem = document.documentElement;

  elem.requestFullscreen({ navigationUI: 'hide' })
    .then(() => {
      fullscreen.value = true;
    })
    .catch((err) => {
      console.error(err);
    });
};

const exitFullscreen = () => {
  document.exitFullscreen();
  fullscreen.value = false;
};

</script>

<template>
  <v-app>
    <v-app-bar app color="primary">
      <v-app-bar-nav-icon @click="drawer = !drawer" />
      <v-toolbar-title>ESP Brew Engine</v-toolbar-title>
      <v-spacer />
      <h5 class="mr-10">Version: {{version}}</h5>

      <v-btn icon v-if="!fullscreen" @click="enterFullscreen">
        <v-icon>{{mdiFullscreen}}</v-icon>
      </v-btn>
      <v-btn icon v-if="fullscreen" @click="exitFullscreen">
        <v-icon>{{mdiFullscreenExit}}</v-icon>
      </v-btn>
    </v-app-bar>

    <v-navigation-drawer v-model="drawer">
      <v-list>

        <v-list-subheader>Brewing</v-list-subheader>
        <v-list-item v-for="[icon, text, route] in linksBrewing" :key="icon" link :to="route">
          <template v-slot:prepend>
            <v-icon>{{ icon }}</v-icon>
          </template>
          <v-list-item-title>{{ text }}</v-list-item-title>
        </v-list-item>

        <v-list-subheader>Settings</v-list-subheader>
        <v-list-item v-for="[icon, text, route] in linksSettings" :key="icon" link :to="route">
          <template v-slot:prepend>
            <v-icon>{{ icon }}</v-icon>
          </template>
          <v-list-item-title>{{ text }}</v-list-item-title>
        </v-list-item>

        <v-list-subheader>Tools</v-list-subheader>
        <v-list-item v-for="[icon, text, route] in linksTools" :key="icon" link :to="route">
          <template v-slot:prepend>
            <v-icon>{{ icon }}</v-icon>
          </template>
          <v-list-item-title>{{ text }}</v-list-item-title>
        </v-list-item>

      </v-list>

    </v-navigation-drawer>

    <v-main style="min-height: 300px;">

      <router-view v-slot="{ Component }">
        <keep-alive include="Control">
          <component :is="Component" />
        </keep-alive>
      </router-view>

    </v-main>
  </v-app>
</template>
