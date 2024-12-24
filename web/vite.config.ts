import * as fs from "node:fs";
import { URL, fileURLToPath } from "node:url";
// Plugins
// eslint-disable-next-line import/order
import vue from "@vitejs/plugin-vue";
import vuetify, { transformAssetUrls } from "vite-plugin-vuetify";

// Utilities
import { defineConfig, loadEnv } from "vite";
import { viteSingleFile } from "vite-plugin-singlefile";

// Read main version from parent and write to .env, seems imposible to include it directly
const packageVersion = fs.readFileSync("../version", "utf8");
const envData = `VITE_APP_VERSION=${packageVersion}`;

fs.writeFileSync(".env", envData);

export default defineConfig(({ command, mode }) => {
  const env = loadEnv(mode, process.cwd(), "");

  return {
    plugins: [
      vue({
        template: { transformAssetUrls },
      }),
      vuetify({
        autoImport: true,
        styles: {
          configFile: "src/styles/settings.scss",
        },
      }),
      viteSingleFile(),
      // {
      //   name: 'build-script',
      //   closeBundle() {
      //     if (command === 'build') {
      //       // add version to service worker, vite doesn't replace vars in js by default
      //       let swFile = fs.readFileSync('dist/sw.js', 'utf8');
      //       swFile = swFile.replaceAll('%VITE_APP_VERSION%', packageVersion);
      //       fs.writeFileSync('dist/sw.js', swFile);
      //     }
      //   },
      // },
    ],
    define: {
      "process.env": env,
    },
    resolve: {
      alias: {
        "@": fileURLToPath(new URL("./src", import.meta.url)),
      },
      extensions: [".js", ".json", ".jsx", ".mjs", ".ts", ".tsx", ".vue"],
    },
    server: {
      port: 3000,
    },
    css: {
      preprocessorOptions: {
        sass: {
          api: "modern",
        },
        scss: {
          api: "modern-compiler", // or "modern"
        },
      },
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
