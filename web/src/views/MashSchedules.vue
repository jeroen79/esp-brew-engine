<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { computed, inject, ref, watch, watchEffect } from 'vue';
import WebConn from '@/helpers/webConn';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { IMashStep, defaultMashStep } from '@/interfaces/IMashStep';
import { useAppStore } from '@/store/app';
import { INotification, defaultNotification } from '@/interfaces/INotification';
import StepEditor from '@/components/StepEditor.vue';
import NotificationEditor from '@/components/NotificationEditor.vue';

const webConn = inject<WebConn>('webConn');

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const appStore = useAppStore();

// copy settings, we don't want them applied until save is clicked
const mashSchedules = ref<Array<IMashSchedule>>([...appStore.mashSchedules]);

const currentName = ref<string>('');
const currentBoil = ref<boolean>(false);
const selectedMashSchedule = ref<IMashSchedule | null>(null);

const getData = async () => {
  await appStore.getMashSchedules();
  mashSchedules.value = [...appStore.mashSchedules];
};

// Steps
const tableStepsData = ref<Array<IMashStep>>([]);
watch(() => selectedMashSchedule.value, () => {
  if (selectedMashSchedule.value == null) {
    tableStepsData.value = [];
    return;
  }

  tableStepsData.value = [...selectedMashSchedule.value.steps];
});

// Notifications
const tableNotificationsData = ref<Array<INotification>>([]);
watch(() => selectedMashSchedule.value, () => {
  if (selectedMashSchedule.value == null) {
    tableNotificationsData.value = [];
    return;
  }

  tableNotificationsData.value = [...selectedMashSchedule.value.notifications];
});

// change name, but copy so user can change it
watchEffect(() => {
  if (selectedMashSchedule.value != null) {
    currentName.value = selectedMashSchedule.value?.name;
    currentBoil.value = selectedMashSchedule.value?.boil;
  }
});

const saveSchedule = async () => {
  // atm schedules are in ram so we can't allow crazy amounts, in the future we will need some kinde of cloud storage
  if ((selectedMashSchedule.value != null && selectedMashSchedule.value.name !== currentName.value) && mashSchedules.value.length >= 10) {
    alert.value = 'Only 10 Schedules allowed!';
    alertType.value = 'warning';
    return;
  }

  const newSchedule:IMashSchedule = {
    name: currentName.value,
    boil: currentBoil.value,
    steps: [...tableStepsData.value],
    notifications: [...tableNotificationsData.value],
  };

  const requestData = {
    command: 'SaveMashSchedule',
    data: newSchedule,
  };

  const result = await webConn?.doPostRequest(requestData);

  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }

  // saved as new, so we refresh to be sure
  getData();
};

const deleteSchedule = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }

  const requestData = {
    command: 'DeleteMashSchedule',
    data: {
      name: selectedMashSchedule.value.name,
    },
  };

  selectedMashSchedule.value = null;

  await webConn?.doPostRequest(requestData);

  getData();
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-row>
        <v-col cols="12" md="3">
          <v-select label="Mash/Boil Schedule" v-model="selectedMashSchedule" :items="mashSchedules" item-title="name" :filled="mashSchedules" clearable return-object />
        </v-col>

      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="currentName" label="Name" />
        </v-col>
        <v-col cols="12" md="3">
          <v-switch v-model="currentBoil" label="Is Boil Schedule" color="red" />
        </v-col>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="saveSchedule"> Save </v-btn>
          <v-btn color="error" class="mt-4 mr-2" @click="deleteSchedule"> Delete </v-btn>
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <StepEditor v-model="tableStepsData" :items-per-page="20" :allow-new="true" />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <NotificationEditor v-model="tableNotificationsData" :items-per-page="20" :allow-new="true" />
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
