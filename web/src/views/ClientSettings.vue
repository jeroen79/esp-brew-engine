<script lang="ts" setup>
import { mdiHelp } from '@mdi/js';
import { onBeforeUnmount, onMounted, ref } from 'vue';
import { IClientSettings } from '@/interfaces/IClientSettings';
import { useClientStore } from '@/store/client';
import { ITitleValue } from '@/interfaces/ITitleValue';

const clientStore = useClientStore();

// copy settings, we don't want them applied until save is clicked
const clientSettings = ref<IClientSettings>({ ...clientStore.clientSettings });

const testMessage = ref<string>('Hi this is a test message');
const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info'>('info');

const voices = ref<Array<ITitleValue>>([]);

const testVoice = async () => {
  if (clientSettings.value.voiceUri == null) {
    return;
  }

  const synth = window.speechSynthesis;
  // the first time we get the voice and store it in a ref

  const foundVoice = synth.getVoices().find((v) => v.voiceURI === clientSettings.value.voiceUri);

  // unable to get a valid voice
  if (foundVoice == null) {
    return;
  }

  const ssu = new SpeechSynthesisUtterance(testMessage.value);
  ssu.voice = foundVoice;
  ssu.rate = clientSettings.value.speechRate;
  ssu.volume = clientSettings.value.speechVolume;
  synth.speak(ssu);
};

const getData = async () => {
  const synth = window.speechSynthesis;

  voices.value = synth.getVoices().sort((a, b) => {
    const aname = a.name.toUpperCase();
    const bname = b.name.toUpperCase();

    if (aname < bname) {
      return -1;
    } if (aname === bname) {
      return 0;
    }
    return +1;
  }).map((voice) => ({
    title: `${voice.name} (${voice.lang})`,
    value: voice.voiceURI,
  }));

  // add empty option
  voices.value.push({ title: 'None', value: null });
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

// we preserve the save logic so client settings are same as server settings
const save = async () => {
  clientStore.clientSettings = { ...clientSettings.value };
  clientStore.saveLocalSettings();
};

</script>

<template>
  <v-container class="spacing-playground pa-6 mt-2" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{ alert }}</v-alert>
    <v-form fast-fail @submit.prevent>

      <v-row>
        <v-col cols="12" md="3">
          <v-switch v-model="clientSettings.beepEnabled" label="Beep" color="green" />
        </v-col>
        <v-col cols="12" md="9">
          <v-slider
            v-model="clientSettings.beepVolume"
            label="Beep Volume (%)"
            step="0.05"
            thumb-label="always"
            max=1
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-switch v-model="clientSettings.speechEnabled" label="Speech" color="green" />
        </v-col>
        <v-col cols="12" md="9">
          <v-slider
            v-model="clientSettings.speechVolume"
            label="Speech Volume (%)"
            step="0.05"
            thumb-label="always"
            max=1
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-select label="Voice" v-model="clientSettings.voiceUri" :filled="voices" :items="voices" />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-slider
            v-model="clientSettings.speechRate"
            label="Speech Rate"
            step="0.05"
            thumb-label="always"
            max=2
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="9">
          <v-text-field label="Test Message" v-model="testMessage" />
        </v-col>
        <v-col cols="3">
          <v-btn variant="outlined" color="success" class="mt-4 mr-2" @click="testVoice"> Test </v-btn>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-switch color="grey" v-model="clientSettings.darkMode" label="Dark Mode"></v-switch>
        </v-col>

      </v-row>

      <v-row>
        <v-col cols="12">
          <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
