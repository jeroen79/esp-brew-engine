<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { computed, inject, onBeforeUnmount, onMounted, ref, watchEffect } from 'vue';
import { VDataTable } from 'vuetify/labs/VDataTable';
import WebConn from '@/helpers/webConn';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { IMashStep } from '@/interfaces/IMashStep';
import { useAppStore } from '@/store/app';

const webConn = inject<WebConn>('webConn');

const appStore = useAppStore();

const mashSchedules = ref<Array<IMashSchedule>>([]);
const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const selectedMashSchedule = ref<IMashSchedule | null>(null);

const defaultStep:IMashStep = {
  index: 0,
  name: 'New Step',
  temperature: 0,
  stepTime: 0,
  time: 0,
  extendStepTimeIfNeeded: true,
};

const editIndex = ref<Number>(-1);
const editedItem = ref<IMashStep>(defaultStep);
const newName = ref<string>('');

const tableItemsPerPage = ref<number>(50);
const tableHeaders = ref<Array<any>>([
  { title: 'Index', key: 'index', align: 'start' },
  { title: 'Name', key: 'name', align: 'start' },
  { title: `Temperature ${appStore.tempUnit}`, key: 'temperature', align: 'end' },
  { title: 'Step Time (min)', key: 'stepTime', align: 'end' },
  { title: 'Extend Step Time if Needed', key: 'extendStepTimeIfNeeded', align: 'end' },
  { title: 'Hold Time (min)', key: 'time', align: 'end' },
  { title: 'Actions', key: 'actions', sortable: false },
]);

const sortBy = ref<Array<any>>([{ key: 'index', order: 'asc' }]);

const getData = async () => {
  const requestData = {
    command: 'GetMashSchedules',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }
  mashSchedules.value = apiResult.data;
};

const tableData:any = computed(() => {
  // wait for init init
  if (selectedMashSchedule.value == null) {
    return [];
  }

  // console.log('steps', selectedMashSchedule.value.steps);
  return [...selectedMashSchedule.value.steps];
});

// change name, but copy so user can change it
watchEffect(() => {
  if (selectedMashSchedule.value != null) {
    newName.value = selectedMashSchedule.value?.name;
  }
});

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

const editItem = async (item:IMashStep) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editIndex.value = selectedMashSchedule.value.steps.indexOf(item);
  editedItem.value = item;
  dialog.value = true;
};

const newItem = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }

  const newStep = { ...defaultStep };
  newStep.index = Math.max(...selectedMashSchedule.value.steps.map((s) => s.index)) + 1;
  selectedMashSchedule.value.steps.push(newStep);

  editedItem.value = newStep;
  dialog.value = true;
};

const openDeleteDialog = async (item:IMashStep) => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  editIndex.value = selectedMashSchedule.value.steps.indexOf(item);
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }
  const index = editIndex.value as number;
  selectedMashSchedule.value.steps.splice(index, 1);
  closeDeleteDialog();
};

const saveSchedule = async () => {
  if (selectedMashSchedule.value == null) {
    return;
  }

  const newSchedule:IMashSchedule = {
    name: newName.value,
    steps: [...selectedMashSchedule.value.steps],
  };

  const requestData = {
    command: 'SaveMashSchedule',
    data: newSchedule,
  };
  await webConn?.doPostRequest(requestData);
  // todo capture result and log errors

  if (selectedMashSchedule.value.name !== newName.value) {
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

  getData();

  await webConn?.doPostRequest(requestData);
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-form fast-fail @submit.prevent>
      <v-row>
        <v-col cols="3" md="3">
          <v-select label="Mash Schedule" v-model="selectedMashSchedule" :items="mashSchedules" item-title="name" :filled="mashSchedules" clearable return-object />
        </v-col>

      </v-row>
      <v-row>
        <v-col cols="3" md="3">
          <v-text-field v-model="newName" label="Name" />
        </v-col>
        <v-col cols="3" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="saveSchedule"> Save </v-btn>
          <v-btn color="error" class="mt-4 mr-2" @click="deleteSchedule"> Delete </v-btn>
        </v-col>
      </v-row>
      <v-row>
        <v-data-table
          :sort-by="sortBy"
          :items-per-page="tableItemsPerPage"
          :headers="tableHeaders"
          :items="tableData"
          density="compact"
          item-value="name"
        >
          <template v-slot:top>
            <v-toolbar density="compact">
              <v-toolbar-title>Mash Schedule</v-toolbar-title>
              <v-spacer />
              <v-btn color="secondary" variant="outlined" class="mr-5" @click="newItem()">
                New Item
              </v-btn>

              <v-dialog v-model="dialog" max-width="500px">
                <v-card>
                  <v-card-title>
                    <span class="text-h5">Edit</span>
                  </v-card-title>

                  <v-card-text>
                    <v-container>
                      <v-row>
                        <v-text-field type="number" v-model.number="editedItem.index" label="Index name" />
                      </v-row>
                      <v-row>
                        <v-text-field v-model="editedItem.name" label="Name" />
                      </v-row>
                      <v-row>
                        <v-text-field type="number" v-model.number="editedItem.temperature" :label="`Temperature ${appStore.tempUnit}`" />
                      </v-row>
                      <v-row>
                        <v-text-field type="number" v-model.number="editedItem.stepTime" label="Step Time (min)" />
                      </v-row>
                      <v-row>
                        <v-checkbox v-model="editedItem.extendStepTimeIfNeeded" label="Extend Step Time if Needed" />
                      </v-row>
                      <v-row>
                        <v-text-field type="number" v-model.number="editedItem.time" label="Hold Time (min)" />
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
              <v-dialog v-model="dialogDelete" max-width="500px">
                <v-card>
                  <v-card-title class="text-h5">Are you sure you want to delete this item?</v-card-title>
                  <v-card-actions>
                    <v-spacer />
                    <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">Cancel</v-btn>
                    <v-btn color="blue-darken-1" variant="text" @click="deleteItemOk">OK</v-btn>
                    <v-spacer />
                  </v-card-actions>
                </v-card>
              </v-dialog>
            </v-toolbar>
          </template>
          <template v-slot:[`item.actions`]="{ item }">
            <v-icon size="small" class="me-2" @click="editItem(item.raw)" :icon="mdiPencil" />
            <v-icon size="small" @click="openDeleteDialog(item.raw)" :icon="mdiDelete" />
          </template>
          <template v-slot:[`item.extendStepTimeIfNeeded`]="{ item }">
            <v-checkbox-btn class="align-right justify-center" v-model="item.columns.extendStepTimeIfNeeded" disabled />
          </template>

        </v-data-table>

      </v-row>
    </v-form>
  </v-container>
</template>
