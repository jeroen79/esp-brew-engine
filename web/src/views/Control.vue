<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount, watch, inject, computed } from 'vue';
import debounce from 'lodash.debounce';
import { Line } from 'vue-chartjs';
import { Chart as ChartJS, Title, Tooltip, Legend, LinearScale, LineElement, PointElement, CategoryScale, TimeScale, Filler } from 'chart.js';
import 'chartjs-adapter-dayjs-4';
import WebConn from '@/helpers/webConn';
import { IDataPacket } from '@/interfaces/IDataPacket';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { IExecutionStep } from '@/interfaces/IExecutionStep';
import { ITempLog } from '@/interfaces/ITempLog';
import { ITempSensor } from '@/interfaces/ITempSensor';

const webConn = inject<WebConn>('webConn');

const status = ref<string>();
const stirStatus = ref<string>();
const temperature = ref<number>();
const outputPercent = ref<number>();
const targetTemperature = ref<number>();
const targetTemperatureSet = ref<number>();
const manualOverrideOutput = ref<number | null>(null);

const intervalId = ref<any>();

const chartOptions = ref<any>(null);

const lastGoodDataDate = ref<number | null>(null);
const lastRunningVersion = ref<number>(0);

const rawData = ref<Array<IDataPacket>>([]);

const mashSchedules = ref<Array<IMashSchedule>>([]);
const tempSensors = ref<Array<ITempSensor>>([]);
const executionSteps = ref<Array<IExecutionStep>>([]);

const selectedMashSchedule = ref<IMashSchedule | null>(null);

const currentTemps = ref<Array<ITempLog>>([]);

const startDateTime = ref<number>();

const stirInterval = ref<Array<number>>([0, 5]);
const stirMax = ref<number>(10);

const dynamicColor = () => {
  const r = Math.floor(Math.random() * 255);
  const g = Math.floor(Math.random() * 255);
  const b = Math.floor(Math.random() * 255);
  return `rgb(${r},${g},${b})`;
};

const chartData = computed(() => {
  // wait for chartjs init
  if (chartOptions.value == null) {
    return null;
  }
  let scheduleData:Array<any> = [];

  if (executionSteps.value != null && executionSteps.value.length > 0) {
    scheduleData = executionSteps.value.map((step:any) => ({
      x: step.time * 1000,
      y: step.temperature,
    }));
  } else if (selectedMashSchedule.value !== null && selectedMashSchedule.value.steps !== null && startDateTime.value !== undefined) {
    // when we have no steps and we are idle we can show the slected mash schedule
    // if the status is idle we can just project the selectes mash shedule
    if (status.value === 'Idle') {
      // sort data on index
      const steps = [...selectedMashSchedule.value.steps];
      steps.sort((a, b) => a.index - b.index);

      let lastTimePoint = startDateTime.value;

      const startPoint = {
        x: lastTimePoint * 1000,
        y: temperature.value,
      };
      scheduleData.push(startPoint);

      steps.forEach((step) => {
        lastTimePoint += (step.stepTime * 60);
        const startStepPoint = {
          x: lastTimePoint * 1000,
          y: step.temperature,
        };
        scheduleData.push(startStepPoint);

        lastTimePoint += (step.time * 60);
        const endStepPoint = {
          x: lastTimePoint * 1000,
          y: step.temperature,
        };
        scheduleData.push(endStepPoint);
      });
    }
  }

  const realData = rawData.value.map((item) => ({
    x: item.time * 1000,
    y: item.temp,
  }));

  // also add the current value, out controller doesn't send identical temp point for performance reasons
  if (lastGoodDataDate.value != null && temperature.value != null) {
    realData.push({
      x: Date.now(),
      y: temperature.value,
    });
  }

  let datasets = [
    {
      label: 'Avg °C',
      backgroundColor: 'rgba(255, 255, 255, 0.7)',
      borderColor: 'rgba(255, 255, 255, 0.9)',
      lineTension: 0,
      fill: false,
      xAxisID: 'xAxis',
      yAxisID: 'yAxis',
      data: realData,
    },
    {
      label: 'Target °C',
      backgroundColor: 'rgba(0, 158, 85, 0.3)',
      borderColor: 'rgba(0, 158, 85, 0.9)',
      lineTension: 0,
      xAxisID: 'xAxis',
      yAxisID: 'yAxis',
      fill: true,
      data: scheduleData,
    },
  ];

  const extraDataSets = currentTemps.value.map((extraSet) => {
    const setData = extraSet.temps.map((temp) => ({
      x: temp.time * 1000,
      y: temp.temp,
    }));

    let label = extraSet.sensor;
    let { color } = extraSet;
    const sensor = tempSensors.value.find((s) => s.id === extraSet.sensor);

    if (sensor !== undefined) {
      label = sensor.name;
      color = sensor.color;
    }

    const dataset = {
      label,
      backgroundColor: color,
      borderColor: color,
      lineWidth: 0.2,
      lineTension: 0,
      xAxisID: 'xAxis',
      yAxisID: 'yAxis',
      fill: false,
      pointRadius: 0,
      data: setData,
    };

    return dataset;
  });

  datasets = [...datasets, ...extraDataSets];

  return {
    labels: [],
    datasets,
  };
});

const getRunningSchedule = async () => {
  const requestData = {
    command: 'GetRunningSchedule',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  executionSteps.value = apiResult.data.steps;

  lastRunningVersion.value = apiResult.data.version;
};

const getData = async () => {
  const requestData = {
    command: 'Data',
    data: {
      LastDate: lastGoodDataDate.value,
    },
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  status.value = apiResult.data.status;
  stirStatus.value = apiResult.data.stirStatus;
  temperature.value = apiResult.data.temp;
  outputPercent.value = apiResult.data.output;
  manualOverrideOutput.value = apiResult.data.manualOverrideOutput;
  targetTemperature.value = apiResult.data.targetTemp;
  lastGoodDataDate.value = apiResult.data.lastLogDateTime;
  const serverRunningVersion = apiResult.data.runningVersion;

  if (status.value === 'Running' && lastRunningVersion.value !== serverRunningVersion) {
    // the schedule has changed, we need to update
    getRunningSchedule();
  }

  const tempData = [...rawData.value, ...apiResult.data.tempLog];

  // sort data, chartjs seems todo weird things otherwise
  tempData.sort((a, b) => a.time - b.time);

  rawData.value = tempData;

  // if there are more then 1 sensor we also get the raw data per sensor (whitout history)
  const timestampSeconds = Math.floor(Date.now() / 1000);

  if (apiResult.data.temps !== null && apiResult.data.temps.length > 1) {
    apiResult.data.temps.forEach((t:any) => {
      // find record in templog and add
      const foundRecord = currentTemps.value.find((ct:any) => ct.sensor === t.sensor);
      if (foundRecord === undefined) {
        const newRecord:ITempLog = {
          sensor: t.sensor,
          color: dynamicColor(),
          temps: [
            {
              time: timestampSeconds,
              temp: t.temp,
            },
          ],
        };

        currentTemps.value.push(newRecord);
      } else {
        foundRecord.temps.push(
          {
            time: timestampSeconds,
            temp: t.temp,
          },
        );
      }
    });
  }

  // we only need to get the mashschedules once
  if (mashSchedules.value == null || mashSchedules.value.length === 0) {
    const requestData2 = {
      command: 'GetMashSchedules',
      data: null,
    };
    const apiResult2 = await webConn?.doPostRequest(requestData2);

    if (apiResult2 === undefined || apiResult2.success === false) {
      return;
    }

    mashSchedules.value = apiResult2.data;
  }

  // we only need to get the tempsensort once
  if (tempSensors.value == null || tempSensors.value.length === 0) {
    const requestData3 = {
      command: 'GetTempSettings',
      data: null,
    };
    const apiResult3 = await webConn?.doPostRequest(requestData3);

    if (apiResult3 === undefined || apiResult3.success === false) {
      return;
    }

    tempSensors.value = apiResult3.data;
  }
};

const changeTargetTemp = async () => {
  if (targetTemperatureSet.value === undefined) {
    return;
  }

  // for some reason value is still a string while ref defined as number, bug in vue?
  const forceInt = parseInt(targetTemperatureSet.value?.toString(), 10);

  const requestData = {
    command: 'SetTemp',
    data: {
      targetTemp: forceInt,
    },
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
};

const changeOverrideOutput = (event:any) => {
  if (event.target.value === undefined) {
    return;
  }

  const forceInt = parseInt(event.target.value.toString(), 10);

  const requestData = {
    command: 'SetOverrideOutput',
    data: {
      output: forceInt,
    },
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
};

const start = async () => {
  const requestData = {
    command: 'Start',
    data: {
      selectedMashSchedule: null as string | null,
    },
  };

  if (selectedMashSchedule.value != null) {
    requestData.data.selectedMashSchedule = selectedMashSchedule.value?.name;
  }

  await webConn?.doPostRequest(requestData);
  // todo capture error
  // reset out running version so we can definitly get the new schedule
  lastRunningVersion.value = 0;
};

const stop = async () => {
  const requestData = {
    command: 'Stop',
    data: null,
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
};

const startStir = async () => {
  const requestData = {
    command: 'StartStir',
    data: {
      max: stirMax.value,
      intervalStart: stirInterval.value[0],
      intervalStop: stirInterval.value[1],
    },
  };

  await webConn?.doPostRequest(requestData);
  // todo capture error
};

const stopStir = async () => {
  const requestData = {
    command: 'StopStir',
    data: null,
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
};

const debounceTargetTemp = debounce(changeTargetTemp, 1000);

watch(() => targetTemperatureSet.value, debounceTargetTemp);

const initChart = () => {
  ChartJS.register(Title, Tooltip, Legend, PointElement, LineElement, TimeScale, LinearScale, CategoryScale, Filler);
  chartOptions.value = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false, // Disable all animations, does weird things when adding data
    scales: {
      xAxis: {
        title: {
          display: true,
          text: 'Time',
          color: '#ffffff',
        },
        type: 'time',
        time: {
          unit: 'minute',
          displayFormats: {
            millisecond: 'HH:mm',
            second: 'HH:mm',
            minute: 'HH:mm',
            hour: 'HH:mm',
            day: 'HH:mm',
            week: 'HH:mm',
            month: 'HH:mm',
            quarter: 'HH:mm',
            year: 'HH:mm',
          },
        },
        ticks: {
          color: '#ffffff',
        },
      },
      yAxis: {
        title: {
          display: true,
          text: '°C',
          color: '#ffffff',
        },
        type: 'linear',
        suggestedMin: 60,
        suggestedMax: 105,
        ticks: {
          color: '#ffffff',
        },
        grid: { color: '#bdbdbc' },
      },
    },
  };
};

onMounted(() => {
  // atm only used to render te schedule at the current time
  const now = new Date();
  startDateTime.value = Math.floor(now.getTime() / 1000);

  intervalId.value = setInterval(() => {
    getData();
  }, 3000);

  initChart();
});

onBeforeUnmount(() => {
  clearInterval(intervalId.value);
});

</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-form fast-fail @submit.prevent>
      <v-row style="height: 50vh">
        <Line v-if="chartData" :options="chartOptions" :data="chartData" />
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="status" readonly label="Status" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="temperature" readonly label="Temperature (°C)" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="targetTemperature" readonly label="Target (°C)" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="targetTemperatureSet" type="number" label="Set Target (°C)" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-select label="Mash Schedule" v-model="selectedMashSchedule" :items="mashSchedules" item-title="name" :filled="mashSchedules" clearable return-object />
        </v-col>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4" block @click="start"> Start </v-btn>

        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="outputPercent" readonly label="Output (%)" />
        </v-col>

      </v-row>
      <v-row>
        <v-col cols="12" md="3" />
        <v-col cols="12" md="3">
          <v-btn color="error" class="mt-4" block @click="stop"> Stop </v-btn> </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model.number="manualOverrideOutput" type="number" label="Override Output (%)" readonly />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field type="number" label="Set Override Output (%)" @change="changeOverrideOutput" />
        </v-col>
      </v-row>
      <div class="text-subtitle-2 mt-4 mb-2">Stir/Pump Control</div>

      <v-divider :thickness="7" />
      <v-row>

        <v-col cols="12" md="">

          <v-range-slider
            v-model="stirInterval"
            label="Interval (min)"
            step="1"
            thumb-label="always"
            :max="stirMax"
          >
            <template v-slot:append>
              <v-text-field
                v-model.number="stirMax"
                hide-details
                single-line
                type="number"
                variant="outlined"
                style="width: 70px"
                density="compact"
                label="Timespan (min)"
              />
            </template>
          </v-range-slider>

        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="stirStatus" readonly label="Status" />
        </v-col>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4" block @click="startStir"> Start </v-btn>
        </v-col>
        <v-col cols="12" md="3">
          <v-btn color="error" class="mt-4" block @click="stopStir"> Stop </v-btn>
        </v-col>
        <v-col cols="12" md="3" />
      </v-row>
    </v-form>
  </v-container>
</template>
