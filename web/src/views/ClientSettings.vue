<script lang="ts" setup>
import { mdiHelp } from '@mdi/js';
import { onBeforeUnmount, onMounted, ref } from 'vue';
import { IClientSettings } from '@/interfaces/IClientSettings';
import { useClientStore } from '@/store/client';
import { ITitleValue } from '@/interfaces/ITitleValue';
import { useI18n } from 'vue-i18n';
const { t } = useI18n({ useScope: 'global' });

const clientStore = useClientStore();

// copy settings, we don't want them applied until save is clicked
const clientSettings = ref<IClientSettings>({ ...clientStore.clientSettings });

const testMessage = ref<string>(t('clientsettings.test_message'));
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
          <v-switch v-model="clientSettings.beepEnabled" :label="$t('clientsettings.beep')" color="green" />
        </v-col>
        <v-col cols="12" md="9">
          <v-slider
            v-model="clientSettings.beepVolume"
            :label="$t('clientsettings.beep_volume')"
            step="0.05"
            thumb-label="always"
            max=1
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-switch v-model="clientSettings.speechEnabled" :label="$t('clientsettings.speech')" color="green" />
        </v-col>
        <v-col cols="12" md="9">
          <v-slider
            v-model="clientSettings.speechVolume"
            :label="$t('clientsettings.speech_volume')"
            step="0.05"
            thumb-label="always"
            max=1
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-select
            :label="$t('clientsettings.voice')"
            v-model="clientSettings.voiceUri"
            :filled="voices"
            :items="voices" />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-slider
            v-model="clientSettings.speechRate"
            :label="$t('clientsettings.speech_rate')"
            step="0.05"
            thumb-label="always"
            max=2
            min=0.1 />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="9">
          <v-text-field :label="$t('clientsettings.test_message')" v-model="testMessage" />
        </v-col>
        <v-col cols="3">
          <v-btn variant="outlined" color="success" class="mt-4 mr-2" @click="testVoice"> {{ $t('clientsettings.test')
          }} </v-btn>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-switch color="grey" v-model="clientSettings.darkMode" :label="$t('clientsettings.darkMode')" />
        </v-col>

      </v-row>

      <v-row>
        <v-col cols="12">
          <v-btn color="success" class="mt-4 mr-2" @click="save"> {{ $t('general.save') }} </v-btn>
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
