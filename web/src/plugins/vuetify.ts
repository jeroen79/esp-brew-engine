/**
 * plugins/vuetify.ts
 *
 * Framework documentation: https://vuetifyjs.com`
 */

// Styles
import 'vuetify/styles';

// Composables
import { createVuetify, ThemeDefinition } from 'vuetify';

// icons via @mdi/js so we only include the used
import { aliases, mdi } from 'vuetify/iconsets/mdi-svg';

const themeBeerPlanetLight: ThemeDefinition = {
  dark: false,
  colors: {
    background: '#FFFFFF',
    surface: '#FFFFFF',
    primary: '#ffa407',
    "dialog-header": '#ffa407',
    'primary-darken-1': '#bf8806',
    secondary: '#000000',
    'secondary-darken-1': '#94791f',
    error: '#B00020',
    info: '#2196F3',
    success: '#4CAF50',
    warning: '#FB8C00',
  },
};

const themeBeerPlanetDark: ThemeDefinition = {
  dark: true,
  colors: {
    background: '#000000',
    surface: '#242320',
    // "on-surface": '#FFFFFF',
    primary: '#ffa407',
    "dialog-header": '#ffa407',
    'primary-darken-1': '#bf8806',
    secondary: '#ffffff',
    'secondary-darken-1': '#94791f',
    error: '#B00020',
    info: '#2196F3',
    success: '#4CAF50',
    warning: '#FB8C00',
  },
};

// https://vuetifyjs.com/en/introduction/why-vuetify/#feature-guides
export default createVuetify({
  theme: {
    defaultTheme: 'dark',
    themes: {
      light: themeBeerPlanetLight,
      dark: themeBeerPlanetDark,
    },
  },
  icons: {
    defaultSet: 'mdi',
    aliases,
    sets: {
      mdi,
    },
  },
});
