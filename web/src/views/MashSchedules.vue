<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { computed, inject, onBeforeUnmount, onMounted, ref, watchEffect } from 'vue';
import { VDataTable } from 'vuetify/labs/VDataTable';
import WebConn from '@/helpers/webConn';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { IMashStep } from '@/interfaces/IMashStep';
import { useAppStore } from '@/store/app';
import { INotification } from '@/interfaces/INotification';

const webConn = inject<WebConn>('webConn');

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const appStore = useAppStore();

// copy settings, we don't want them applied until save is clicked
const mashSchedules = ref<Array<IMashSchedule>>([...appStore.mashSchedules]);

const editStepDialog = ref<boolean>(false);
const editNotificationDialog = ref<boolean>(false);
const stepDialogDelete = ref<boolean>(false);
const notificationDialogDelete = ref<boolean>(false);

const selectedMashSchedule = ref<IMashSchedule | null>(null);

const defaultStep:IMashStep = {
  index: 0,
  name: 'New Step',
  temperature: 0,
  stepTime: 0,
  time: 0,
  extendStepTimeIfNeeded: true,
};

const editStepsIndex = ref<Number>(-1);
const editedStepsItem = ref<IMashStep>(defaultStep);
const currentName = ref<string>('');
const currentBoil = ref<boolean>(false);

const tableItemsPerPage = ref<number>(50);
const tableStepsHeaders = ref<Array<any>>([
  { title: 'Index', key: 'index', align: 'start' },
  { title: 'Name', key: 'name', align: 'start' },
  { title: `Temperature ${appStore.tempUnit}`, key: 'temperature', align: 'end' },
  { title: 'Step Time (min)', key: 'stepTime', align: 'end' },
  { title: 'Extend Step Time if Needed', key: 'extendStepTimeIfNeeded', align: 'end' },
  { title: 'Hold Time (min)', key: 'time', align: 'end' },
  { title: 'Actions', key: 'actions', sortable: false },
]);

const tableStepsData:any = computed(() => {
  // wait for init init
  if (selectedMashSchedule.value == null) {
    return [];
  }

  return [...selectedMashSchedule.value.steps];
});

const defaultNotification:INotification = {
  name: 'New Notification',
  message: '',
  timeFromStart: 0,
  timePoint: 0,
  buzzer: true,
};

const tableNotificationsHeaders = ref<Array<any>>([
  { title: 'Name', key: 'name', align: 'start' },
  { title: 'Time from Start (min)', key: 'timeFromStart', align: 'start' },
  { title: 'Buzzer', key: 'buzzer', align: 'start' },
  { title: 'Actions', key: 'actions', sortable: false },
]);

const sortStepsBy = ref<Array<any>>([{ key: 'index', order: 'asc' }]);
const sortNotificationsBy = ref<Array<any>>([{ key: 'time', order: 'asc' }]);

const editNotificationsIndex = ref<Number>(-1);
const editedNotificationsItem = ref<INotification>(defaultNotification);

const getData = async () => {
  await appStore.getMashSchedules();
  mashSchedules.value = [...appStore.mashSchedules];
};

const tableNotificationsData:any = computed(() => {
  // wait for init init
  if (selectedMashSchedule.value == null) {
    return [];
  }

  // console.log('steps', selectedMashSchedule.value.steps);
  return [...selectedMashSchedule.value.notifications];
});

// change name, but copy so user can change it
watchEffect(() => {
  if (selectedMashSchedule.value != null) {
    currentName.value = selectedMashSchedule.value?.name;
    currentBoil.value = selectedMashSchedule.value?.boil;
  }
});

const closeDialog = async () => {
  editStepDialog.value = false;
  editNotificationDialog.value = false;
};

const closeDeleteDialog = async () => {
  stepDialogDelete.value = false;
  notificationDialogDelete.value = false;
};

const editStepsItem = async (item:IMashStep) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editStepsIndex.value = selectedMashSchedule.value.steps.indexOf(item);
  editedStepsItem.value = item;
  editStepDialog.value = true;
};

const newStep = async () => {
  if (selectedMashSchedule.value == null) { // create new from scratch when user stats adding steps
    selectedMashSchedule.value = {
      name: 'New',
      boil: false,
      steps: [],
      notifications: [],
    };
  }

  const step = { ...defaultStep };

  // increment index if there are already steps, otherwise default 0 is ok
  if (selectedMashSchedule.value.steps.length > 0) {
    step.index = Math.max(...selectedMashSchedule.value.steps.map((s) => s.index)) + 1;
  }

  selectedMashSchedule.value.steps.push(step);

  editedStepsItem.value = step;
  editStepDialog.value = true;
};

const openStepsDeleteDialog = async (item:IMashStep) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editStepsIndex.value = selectedMashSchedule.value.steps.indexOf(item);
  editedStepsItem.value = item;
  stepDialogDelete.value = true;
};

const stepDeleteItemOk = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  const index = editStepsIndex.value as number;
  selectedMashSchedule.value.steps.splice(index, 1);
  closeDeleteDialog();
};

const editNotificationsItem = async (item:INotification) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editNotificationsIndex.value = selectedMashSchedule.value.notifications.indexOf(item);
  editedNotificationsItem.value = item;
  editNotificationDialog.value = true;
};

const newNotification = async () => {
  if (selectedMashSchedule.value == null) { // create new from scratch when user stats adding steps
    selectedMashSchedule.value = {
      name: 'New',
      boil: false,
      steps: [],
      notifications: [],
    };
  }

  const notification = { ...defaultNotification };
  selectedMashSchedule.value.notifications.push(notification);

  editedNotificationsItem.value = notification;
  editNotificationDialog.value = true;
};

const openNotificationsDeleteDialog = async (item:INotification) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editNotificationsIndex.value = selectedMashSchedule.value.notifications.indexOf(item);
  editedNotificationsItem.value = item;
  notificationDialogDelete.value = true;
};

const notificationDeleteItemOk = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  const index = editNotificationsIndex.value as number;
  selectedMashSchedule.value.notifications.splice(index, 1);
  closeDeleteDialog();
};

const saveSchedule = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }

  // atm schedules are in ram so we can't allow crazy amounts, in the future we will need some kinde of cloud storage
  if (selectedMashSchedule.value.name !== currentName.value && mashSchedules.value.length >= 10) {
    alert.value = 'Only 10 Schedules allowed!';
    alertType.value = 'warning';
    return;
  }

  const newSchedule:IMashSchedule = {
    name: currentName.value,
    boil: currentBoil.value,
    steps: [...selectedMashSchedule.value.steps],
    notifications: [...selectedMashSchedule.value.notifications],
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

  if (selectedMashSchedule.value.name !== currentName.value) {
    // saved as new, so we refresh to be sure
    getData();
  }
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
          <v-data-table
            :sort-by="sortStepsBy"
            :items-per-page="tableItemsPerPage"
            :headers="tableStepsHeaders"
            :items="tableStepsData"
            density="compact"
            item-value="name"
          >
            <template v-slot:top>
              <v-toolbar density="compact">
                <v-toolbar-title>Steps</v-toolbar-title>
                <v-spacer />
                <v-btn color="secondary" variant="outlined" class="mr-5" @click="newStep()">
                  New Step
                </v-btn>

                <v-dialog v-model="editStepDialog" max-width="500px">
                  <v-card>
                    <v-card-title>
                      <span class="text-h5">Edit</span>
                    </v-card-title>

                    <v-card-text>
                      <v-container>
                        <v-row>
                          <v-text-field type="number" v-model.number="editedStepsItem.index" label="Index" />
                        </v-row>
                        <v-row>
                          <v-text-field v-model="editedStepsItem.name" label="Name" />
                        </v-row>
                        <v-row>
                          <v-text-field type="number" v-model.number="editedStepsItem.temperature" :label="`Temperature ${appStore.tempUnit}`" />
                        </v-row>
                        <v-row>
                          <v-text-field type="number" v-model.number="editedStepsItem.stepTime" label="Step Time (min)" />
                        </v-row>
                        <v-row>
                          <v-checkbox v-model="editedStepsItem.extendStepTimeIfNeeded" label="Extend Step Time if Needed" />
                        </v-row>
                        <v-row>
                          <v-text-field type="number" v-model.number="editedStepsItem.time" label="Hold Time (min)" />
                        </v-row>
                      </v-container>
                    </v-card-text>

                    <v-card-actions>
                      <v-spacer />
                      <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                        Close
                      </v-btn>
                    </v-card-actions>
                  </v-card>
                </v-dialog>
                <v-dialog v-model="stepDialogDelete" max-width="500px">
                  <v-card>
                    <v-card-title class="text-h5">Are you sure you want to delete this Step?</v-card-title>
                    <v-card-actions>
                      <v-spacer />
                      <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">Cancel</v-btn>
                      <v-btn color="blue-darken-1" variant="text" @click="stepDeleteItemOk">OK</v-btn>
                      <v-spacer />
                    </v-card-actions>
                  </v-card>
                </v-dialog>
              </v-toolbar>
            </template>
            <template v-slot:[`item.actions`]="{ item }">
              <v-icon size="small" class="me-2" @click="editStepsItem(item.raw)" :icon="mdiPencil" />
              <v-icon size="small" @click="openStepsDeleteDialog(item.raw)" :icon="mdiDelete" />
            </template>
            <template v-slot:[`item.extendStepTimeIfNeeded`]="{ item }">
              <v-checkbox-btn class="align-right justify-center" v-model="item.columns.extendStepTimeIfNeeded" disabled />
            </template>

          </v-data-table>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12">
          <v-data-table
            :sort-by="sortNotificationsBy"
            :items-per-page="tableItemsPerPage"
            :headers="tableNotificationsHeaders"
            :items="tableNotificationsData"
            density="compact"
            item-value="name"
          >
            <template v-slot:top>
              <v-toolbar density="compact">
                <v-toolbar-title>Notifications</v-toolbar-title>
                <v-spacer />
                <v-btn color="secondary" variant="outlined" class="mr-5" @click="newNotification()">
                  New Notification
                </v-btn>

                <v-dialog v-model="editNotificationDialog" max-width="500px">
                  <v-card>
                    <v-card-title>
                      <span class="text-h5">Edit</span>
                    </v-card-title>

                    <v-card-text>
                      <v-container>
                        <v-row>
                          <v-text-field maxlength="25" v-model="editedNotificationsItem.name" label="Name" />
                        </v-row>
                        <v-row>
                          <v-text-field type="number" v-model.number="editedNotificationsItem.timeFromStart" label="Time from Start (min)" />
                        </v-row>
                        <v-row>
                          <v-switch v-model="editedNotificationsItem.buzzer" label="Buzzer" color="red" />
                        </v-row>
                        <v-row>
                          <v-textarea v-model="editedNotificationsItem.message" label="Message" />
                        </v-row>
                      </v-container>
                    </v-card-text>

                    <v-card-actions>
                      <v-spacer />
                      <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                        Close
                      </v-btn>
                    </v-card-actions>
                  </v-card>
                </v-dialog>
                <v-dialog v-model="notificationDialogDelete" max-width="500px">
                  <v-card>
                    <v-card-title class="text-h5">Are you sure you want to delete this Notification?</v-card-title>
                    <v-card-actions>
                      <v-spacer />
                      <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">Cancel</v-btn>
                      <v-btn color="blue-darken-1" variant="text" @click="notificationDeleteItemOk">OK</v-btn>
                      <v-spacer />
                    </v-card-actions>
                  </v-card>
                </v-dialog>
              </v-toolbar>
            </template>
            <template v-slot:[`item.actions`]="{ item }">
              <v-icon size="small" class="me-2" @click="editNotificationsItem(item.raw)" :icon="mdiPencil" />
              <v-icon size="small" @click="openNotificationsDeleteDialog(item.raw)" :icon="mdiDelete" />
            </template>
            <template v-slot:[`item.buzzer`]="{ item }">
              <v-checkbox-btn class="align-right justify-center" v-model="item.columns.buzzer" disabled />
            </template>

          </v-data-table>
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
