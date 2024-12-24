/**
 * main.ts
 *
 * Bootstraps Vuetify and other plugins then mounts the App`
 */

// Composables
import { createApp } from "vue";

import { createI18n } from "vue-i18n";

// Import translations
import de from "../locales/de.json";
import en from "../locales/en.json";
import nl from "../locales/nl.json";

// Pina Store
import { createPinia } from "pinia";

// Plugins
import { registerPlugins } from "@/plugins";
import App from "./App.vue";
import { getBrowserLocale } from "./helpers/locale";
import WebConn from "./helpers/webConn";
import { useAppStore } from "./store/app";

// Configure i18n
const i18n = createI18n({
  locale: getBrowserLocale({ countryCodeOnly: true }),
  fallbackLocale: "en",
  messages: { de, en, nl },
  legacy: false,
});

const pinia = createPinia();
const app = createApp(App);

registerPlugins(app);
app.use(pinia);

let rootUrl = `${window.location.origin}/`;

// Swap for development, so we can run localy but send api to brewengine
if (import.meta.env.MODE === "development") {
  rootUrl = "http://brewengine/";
}

const webConn = new WebConn(rootUrl);
app.provide("webConn", webConn);

// Call the store so it can already get system settings
const appStore = useAppStore();
appStore.rootUrl = rootUrl;
appStore.getSystemSettings();

app.use(i18n);
app.mount("#app");
