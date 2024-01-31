/* eslint-disable import/prefer-default-export */
// Utilities
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useLoginStore = defineStore('app', () => {
  const token = ref(0);
  const username = ref(null);
  const nickname = ref('Guest');
  const email = ref('guest@beerplanet.net');
  const avatar = ref<string | null>(null);
  const loggedIn = ref<boolean>(false);

  return {
    token, username, nickname, email, avatar, loggedIn,
  };
});
