// Plugins
import type { App } from "vue";
import router from "../router";
import pinia from "../store";
import vuetify from "./vuetify";
import { loadFonts } from "./webfontloader";

// Types

export function registerPlugins(app: App) {
  loadFonts();
  app.use(vuetify).use(router).use(pinia);
}
