/**
 * main.ts
 *
 * Bootstraps Vuetify and other plugins then mounts the App`
 */

// Components

// Composables
import { createApp } from 'vue';

// Pina Store
import { createPinia } from 'pinia';

// Plugins
import { registerPlugins } from '@/plugins';
import App from './App.vue';
import WebConn from './helpers/webConn';
import { useAppStore } from './store/app';

const pinia = createPinia();
const app = createApp(App);

registerPlugins(app);
app.use(pinia);

let rootUrl = `${window.location.origin}/`;

// swap for development, so we can run localy but send api to brewengine
if (import.meta.env.MODE === 'development') {
  rootUrl = 'http://brewengine/';
}

const webConn = new WebConn(rootUrl);
app.provide('webConn', webConn);

// call the store so it can already get system settings
const appStore = useAppStore();
appStore.rootUrl = rootUrl;
appStore.getSystemSettings();

app.mount('#app');
