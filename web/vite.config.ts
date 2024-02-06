// Plugins
import { fileURLToPath, URL } from 'node:url';
import * as fs from 'fs';
import vue from '@vitejs/plugin-vue';
import vuetify, { transformAssetUrls } from 'vite-plugin-vuetify';

// Utilities
import { defineConfig, loadEnv } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

// Read main version from parent and write to .env, seems imposible to include it directly
const packageVersion = fs.readFileSync('../version', 'utf8');
const envData = `VITE_APP_VERSION=${packageVersion}`;

fs.writeFileSync('.env', envData);

export default defineConfig(({ command, mode }) => {
  const env = loadEnv(mode, process.cwd(), '');

  return {
    plugins: [
      vue({
        template: { transformAssetUrls },
      }),
      // https://github.com/vuetifyjs/vuetify-loader/tree/next/packages/vite-plugin
      vuetify({
        autoImport: true,
        styles: {
          configFile: 'src/styles/settings.scss',
        },
      }),
      viteSingleFile(),
    ],
    define: {
      'process.env': env,
    },
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url)),
      },
      extensions: [
        '.js',
        '.json',
        '.jsx',
        '.mjs',
        '.ts',
        '.tsx',
        '.vue',
      ],
    },
    server: {
      port: 3000,
    },
    // build: {
    //   rollupOptions: {
    //     output: {
    //       manualChunks: undefined,
    //     },
    //   },
    // },
  };
});

// // https://vitejs.dev/config/
// export default defineConfig({
//   plugins: [
//     vue({
//       template: { transformAssetUrls },
//     }),
//     // https://github.com/vuetifyjs/vuetify-loader/tree/next/packages/vite-plugin
//     vuetify({
//       autoImport: true,
//       styles: {
//         configFile: 'src/styles/settings.scss',
//       },
//     }),
//     viteSingleFile(),
//   ],
//   define: {
//     'process.env': { PACKAGE_VERSION: '1.0.5' },
//   },
//   resolve: {
//     alias: {
//       '@': fileURLToPath(new URL('./src', import.meta.url)),
//     },
//     extensions: [
//       '.js',
//       '.json',
//       '.jsx',
//       '.mjs',
//       '.ts',
//       '.tsx',
//       '.vue',
//     ],
//   },
//   server: {
//     port: 3000,
//   },
//   // build: {
//   //   rollupOptions: {
//   //     output: {
//   //       manualChunks: undefined,
//   //     },
//   //   },
//   // },
// });
