<script lang="ts" setup>
import { IMashStep, defaultMashStep } from '@/interfaces/IMashStep';
import { mdiDelete, mdiPencil } from '@mdi/js';
import { useAppStore } from '@/store/app';
import { ref, watch } from 'vue';

const appStore = useAppStore();

const props = defineProps({
  label: { type: String, required: false, default: 'Steps' },
  itemsPerPage: { type: Number, required: false, default: 50 },
  allowNew: { type: Boolean, required: false, default: true },
});

// steps as 2-way data with v-model
const steps = defineModel<Array<IMashStep>>({ required: true });

const itemsPerPage = ref<number>(props.itemsPerPage);
watch(() => props.itemsPerPage, () => {
  itemsPerPage.value = props.itemsPerPage;
});

const allowNew = ref<boolean>(props.allowNew);
watch(() => props.allowNew, () => {
  allowNew.value = props.allowNew;
});

// Steps
const tableStepsHeaders = ref<Array<any>>([
  { title: 'Index', key: 'index', align: 'start' },
  { title: 'Name', key: 'name', align: 'start' },
  { title: `Temperature ${appStore.tempUnit}`, key: 'temperature', align: 'end' },
  { title: 'Step Time (min)', key: 'stepTime', align: 'end' },
  { title: 'Extend Step Time if Needed', key: 'extendStepTimeIfNeeded', align: 'end' },
  { title: 'Hold Time (min)', key: 'time', align: 'end' },
  { title: 'Actions', key: 'actions', sortable: false },
]);
const sortStepsBy = ref<Array<any>>([{ key: 'index', order: 'asc' }]);

const showStepDialog = ref<boolean>(false);
const showStepDeleteDialog = ref<boolean>(false);
const editingStep = ref<IMashStep>(defaultMashStep);

const closeDialog = async () => {
  showStepDialog.value = false;
};

const closeDeleteDialog = async () => {
  showStepDeleteDialog.value = false;
};

const editStep = async (item:IMashStep) => {
  editingStep.value = item;
  showStepDialog.value = true;
};

const newStep = async () => {
  const step = { ...defaultMashStep };

  // increment index if there are already steps, otherwise default 0 is ok
  if (steps.value.length > 0) {
    step.index = Math.max(...steps.value.map((s) => s.index)) + 1;
  }

  steps.value.push(step);

  editingStep.value = step;
  showStepDialog.value = true;
};

const openStepsDeleteDialog = async (item:IMashStep) => {
  editingStep.value = item;
  showStepDeleteDialog.value = true;
};

const stepDeleteItemOk = async () => {
  steps.value = steps.value.filter((s) => s !== editingStep.value);

  closeDeleteDialog();
};
</script>

<template>
  <v-data-table
    :sort-by="sortStepsBy"
    :items-per-page="itemsPerPage"
    :headers="tableStepsHeaders"
    :items="steps"
    density="compact"
    item-value="name"
  >
    <template v-slot:top>
      <v-toolbar density="compact">
        <v-toolbar-title>{{props.label}}</v-toolbar-title>
        <v-spacer />
        <v-btn color="secondary" variant="outlined" class="mr-5" v-if="allowNew" @click="newStep()">
          New Step
        </v-btn>

        <v-dialog v-model="showStepDialog" max-width="500px">
          <v-card>
            <v-card-title>
              <span class="text-h5">Edit</span>
            </v-card-title>

            <v-card-text>
              <v-container>
                <v-row>
                  <v-text-field type="number" v-model.number="editingStep.index" label="Index" />
                </v-row>
                <v-row>
                  <v-text-field v-model="editingStep.name" label="Name" />
                </v-row>
                <v-row>
                  <v-text-field type="number" v-model.number="editingStep.temperature" :label="`Temperature ${appStore.tempUnit}`" />
                </v-row>
                <v-row>
                  <v-text-field type="number" v-model.number="editingStep.stepTime" label="Step Time (min)" />
                </v-row>
                <v-row>
                  <v-checkbox v-model="editingStep.extendStepTimeIfNeeded" label="Extend Step Time if Needed" />
                </v-row>
                <v-row>
                  <v-text-field type="number" v-model.number="editingStep.time" label="Hold Time (min)" />
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
        <v-dialog v-model="showStepDeleteDialog" max-width="500px">
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
      <v-icon size="small" class="me-2" @click="editStep(item)" :icon="mdiPencil" />
      <v-icon size="small" @click="openStepsDeleteDialog(item)" :icon="mdiDelete" />
    </template>
    <template v-slot:[`item.extendStepTimeIfNeeded`]="{ item }">
      <v-checkbox-btn class="align-right justify-center" v-model="item.extendStepTimeIfNeeded" disabled />
    </template>

  </v-data-table>
</template>
