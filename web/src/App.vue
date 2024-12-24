<script lang="ts" setup>
import { onMounted, watch } from "vue";
import { useClientStore } from "./store/client";
import { useTheme } from "vuetify";

const clientStore = useClientStore();
const theme = useTheme();

onMounted(() => {
  theme.global.name.value = clientStore.clientSettings.darkMode ? "dark" : "light";
});

watch(
  () => clientStore.clientSettings.darkMode,
  () => {
    theme.global.name.value = clientStore.clientSettings.darkMode ? "dark" : "light";
  },
);
</script>

<template>
  <router-view />
</template>
