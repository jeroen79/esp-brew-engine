<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { inject, onBeforeUnmount, onMounted, ref } from 'vue';
import { IHeater } from '@/interfaces/IHeater';
import WebConn from '@/helpers/webConn';
import { useI18n } from 'vue-i18n'
const { t } = useI18n({ useScope: 'global' })

const webConn = inject<WebConn>('webConn');

const HeaterConfigs = ref<Array<IHeater>>([]);

const tableHeaders = ref<Array<any>>([
  { title: t('heaterSettings.name'), key: 'name', align: 'start' },
  { title: t('heaterSettings.pin'), key: 'pinNr', align: 'start' },
  { title: t('heaterSettings.preference'), key: 'preference', align: 'start' },
  { title: t('heaterSettings.power'), key: 'watt', align: 'start' },
  { title: t('heaterSettings.use_for_mash'), key: 'useForMash', align: 'end' },
  { title: t('heaterSettings.use_for_boil'), key: 'useForBoil', align: 'end' },
  { title: t('heaterSettings.actions'), key: 'actions', align: 'end', sortable: false },
]);

const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info'>('info');

const defaultHeater: IHeater = {
  id: 0,
  name: t('heaterSettings.new_heater'),
  pinNr: 0,
  preference: 0,
  watt: 0,
  useForMash: true,
  useForBoil: true,
};

const editedItem = ref<IHeater>(defaultHeater);

const getData = async () => {
  const requestData = {
    command: 'GetHeaterSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  HeaterConfigs.value = apiResult.data;
};

const newItem = async () => {
  const newHeater = { ...defaultHeater };
  newHeater.id = Math.max(...HeaterConfigs.value.map((s) => s.id)) + 1;
  HeaterConfigs.value.push(newHeater);

  editedItem.value = newHeater;
  dialog.value = true;
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

const editItem = async (item: IHeater) => {
  editedItem.value = item;
  dialog.value = true;
};

const openDeleteDialog = async (item: IHeater) => {
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  // filter out deleted
  HeaterConfigs.value = HeaterConfigs.value.filter((s) => s.id !== editedItem.value.id);
  closeDeleteDialog();
};

const save = async () => {
  if (HeaterConfigs.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveHeaterSettings',
    data: HeaterConfigs.value,
  };

  const result = await webConn?.doPostRequest(requestData);

  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{ alert }}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-data-table
        :headers="tableHeaders"
        :items="HeaterConfigs"
        density="compact"
        item-value="name">
        <template v-slot:top>
          <v-toolbar density="compact">
            <v-toolbar-title>{{t('heaterSettings.heater_configurations')}}</v-toolbar-title>
            <v-spacer />
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="newItem()">
              {{t('heaterSettings.new_heater')}}
            </v-btn>
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="getData()">
              {{t('heaterSettings.refresh')}}
            </v-btn>

            <v-dialog v-model="dialog" max-width="500px">
              <v-card>
                <v-toolbar density="compact" color="dialog-header">
                  <v-toolbar-title>{{t('heaterSettings.edit')}}</v-toolbar-title>
                </v-toolbar>

                <v-card-text>
                  <v-container>
                    <v-row>
                      <v-text-field v-model="editedItem.name" :label="t('heaterSettings.name')" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.pinNr" :label="t('heaterSettings.pin')" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.preference" :label="t('heaterSettings.preference')" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.watt" :label="t('heaterSettings.power')" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForMash" :label="t('heaterSettings.use_for_mash')" color="orange" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForBoil" :label="t('heaterSettings.use_for_boil')" color="red" />
                    </v-row>
                  </v-container>
                </v-card-text>

                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                    {{t('heaterSettings.close')}}
                  </v-btn>
                </v-card-actions>
              </v-card>
            </v-dialog>
            <v-dialog v-model="dialogDelete" max-width="500px">
              <v-card>
                <v-card-title class="text-h5">{{t('heaterSettings.delete')}}</v-card-title>
                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">{{t('heaterSettings.cancel')}}</v-btn>
                  <v-btn color="blue-darken-1" variant="text" @click="deleteItemOk">{{t('heaterSettings.ok')}}</v-btn>
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
        <template v-slot:[`item.useForMash`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.useForMash" disabled />
        </template>
        <template v-slot:[`item.useForBoil`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.useForBoil" disabled />
        </template>
      </v-data-table>

      <v-btn color="success" class="mt-4 mr-2" @click="save"> {{t('heaterSettings.save')}} </v-btn>

    </v-form>
  </v-container>
</template>
