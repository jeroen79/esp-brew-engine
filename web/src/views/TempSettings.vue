<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount, inject } from 'vue';
import { mdiPencil, mdiDelete, mdiPalette } from '@mdi/js';
import WebConn from '@/helpers/webConn';
import { ITempSensor } from '@/interfaces/ITempSensor';
import { useI18n } from 'vue-i18n'
const { t } = useI18n({ useScope: 'global' })

const webConn = inject<WebConn>('webConn');

const tempSensors = ref<Array<ITempSensor>>([]);

const tableHeaders = ref<Array<any>>([
  { title: t('tempSettings.id'), key: 'id', align: 'start' },
  { title: t('tempSettings.name'), key: 'name', align: 'start' },
  { title: t('tempSettings.color'), key: 'color', align: 'start' },
  { title: t('tempSettings.compensate_abs'), key: 'compensateAbsolute', align: 'start' },
  { title: t('tempSettings.compensate_rel'), key: 'compensateRelative', align: 'start' },
  { title: t('tempSettings.show'), key: 'show', align: 'end' },
  { title: t('tempSettings.use_for_control'), key: 'useForControl', align: 'end' },
  { title: t('tempSettings.connected'), key: 'connected', align: 'end' },
  { title: t('tempSettings.last_temp'), key: 'lastTemp', align: 'end' },
  { title: t('tempSettings.actions'), key: 'actions', align: 'end', sortable: false },
]);

const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info'>('info');

const defaultSensor: ITempSensor = {
  id: '',
  name: t('tempSettings.new_sensor'),
  color: '#ffffff',
  useForControl: false,
  show: false,
  connected: false,
  compensateAbsolute: 0.0,
  compensateRelative: 1,
  lastTemp: 0,
};

const editedItem = ref<ITempSensor>(defaultSensor);

const getData = async () => {
  const requestData = {
    command: 'GetTempSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  tempSensors.value = apiResult.data;
};

const detectTempSensors = async () => {
  const requestData = {
    command: 'DetectTempSensors',
    data: null,
  };

  alert.value = t("tempSettings.msg_scan");
  alertType.value = 'info';

  const apiResult = await webConn?.doPostRequest(requestData);

  alert.value = ''; // clear alert

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  // after successfull detect get new data
  getData();
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const closeDialog = async () => {
  dialog.value = false;
};

const closeDeleteDialog = async () => {
  dialogDelete.value = false;
};

const editItem = async (item: ITempSensor) => {
  editedItem.value = item;
  dialog.value = true;
};

const openDeleteDialog = async (item: ITempSensor) => {
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  // filter out deleted
  tempSensors.value = tempSensors.value.filter((s) => s.id !== editedItem.value.id);
  closeDeleteDialog();
};

const save = async () => {
  if (tempSensors.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveTempSettings',
    data: tempSensors.value,
  };

  await webConn?.doPostRequest(requestData);
  // todo capture result and log errors
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{ alert }}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-data-table
        :headers="tableHeaders"
        :items="tempSensors"
        density="compact"
        item-value="name">
        <template v-slot:top>
          <v-toolbar density="compact">
            <v-toolbar-title>{{t('tempSettings.temp_sensors')}}</v-toolbar-title>
            <v-spacer />
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="getData()">
              {{t('tempSettings.refresh')}}
            </v-btn>
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="detectTempSensors()">
              {{t('tempSettings.detect')}}
            </v-btn>

            <v-dialog v-model="dialog" max-width="500px">
              <v-card>
                <v-toolbar density="compact" color="dialog-header">
                  <v-toolbar-title>{{t('tempSettings.edit')}}</v-toolbar-title>
                </v-toolbar>

                <v-card-text>
                  <v-container>
                    <v-row>
                      <v-text-field v-model="editedItem.name" :label='t("tempSettings.name")' />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.show" :label='t("tempSettings.show")' color="green" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForControl" :label='t("tempSettings.enabled")' color="red" />
                    </v-row>
                    <v-row>
                      <v-color-picker v-model="editedItem.color" hide-inputs :label='t("tempSettings.color")' />
                    </v-row>
                    <v-row>
                      <v-text-field type="number" v-model.number="editedItem.compensateAbsolute" :label='t("tempSettings.compensate_abs")' />
                    </v-row>
                    <v-row>
                      <v-text-field type="number" v-model.number="editedItem.compensateRelative" :label='t("tempSettings.compensate_rel")' />
                    </v-row>
                  </v-container>
                </v-card-text>

                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                    {{t('tempSettings.close')}}
                  </v-btn>
                </v-card-actions>
              </v-card>
            </v-dialog>
            <v-dialog v-model="dialogDelete" max-width="500px">
              <v-card>
                <v-card-title class="text-h5">{{t('tempSettings.delete')}}</v-card-title>
                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">{{t('tempSettings.cancel')}}</v-btn>
                  <v-btn color="blue-darken-1" variant="text" @click="deleteItemOk">{{t('tempSettings.ok')}}</v-btn>
                  <v-spacer />
                </v-card-actions>
              </v-card>
            </v-dialog>
          </v-toolbar>
        </template>
        <template v-slot:[`item.actions`]="{ item }">
          <v-icon size="small" class="me-2" @click="editItem(item)" :icon="mdiPencil" />
          <v-icon size="small" @click="openDeleteDialog(item)" :icon="mdiDelete" />
        </template>
        <template v-slot:[`item.useForControl`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.useForControl" disabled />
        </template>
        <template v-slot:[`item.show`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.show" disabled />
        </template>
        <template v-slot:[`item.connected`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.connected" disabled />
        </template>
        <template v-slot:[`item.color`]="{ item }">
          <v-icon size="small" class="me-2" :icon="mdiPalette" :color="item.color" />
        </template>
      </v-data-table>

      <v-btn color="success" class="mt-4 mr-2" @click="save"> {{t('tempSettings.save')}} </v-btn>

    </v-form>
  </v-container>
</template>
