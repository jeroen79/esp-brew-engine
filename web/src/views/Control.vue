<script lang="ts" setup>
import { mdiCheckCircle, mdiCloseCircle } from '@mdi/js';
import { CategoryScale, Chart as ChartJS, Filler, Legend, LineElement, LinearScale, PointElement, TimeScale, Title, Tooltip } from 'chart.js';
import TemperatureScale from '@/enums/TemperatureScale';
import WebConn from '@/helpers/webConn';
import { IDataPacket } from '@/interfaces/IDataPacket';
import { IExecutionStep } from '@/interfaces/IExecutionStep';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { ITempLog } from '@/interfaces/ITempLog';
import { ITempSensor } from '@/interfaces/ITempSensor';
import { useAppStore } from '@/store/app';
import 'chartjs-adapter-dayjs-4';
import annotationPlugin from 'chartjs-plugin-annotation';
import debounce from 'lodash.debounce';
import { computed, inject, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { Line } from 'vue-chartjs';
import { INotification } from '@/interfaces/INotification';
import { useClientStore } from '@/store/client';
import { useI18n } from 'vue-i18n'
const { t } = useI18n({ useScope: 'global' })

const webConn = inject<WebConn>('webConn');

const appStore = useAppStore();
const clientStore = useClientStore();

const status = ref<string>();
const stirStatus = ref<string>();
const temperature = ref<number>();
const outputPercent = ref<number>();
const targetTemperature = ref<number>();
const targetTemperatureChanged = ref<boolean>(false);
const manualOverrideOutput = ref<number | null>(null);
const manualOverrideOutputChanged= ref<boolean>(false);
const inOverTime = ref<boolean>(false);

const intervalId = ref<any>();

const notificationDialog = ref<boolean>(false);
const notificationDialogTitle = ref<string>('');
const notificationDialogText = ref<string>('');

const notificationTimeouts = ref<Array<number>>([]);
const notificationFirstStart = ref(true); //first not at 0 sec is already done in backend, wich is correct, but we do need to show it in web so we need a boolean to check this

const chartInitDone = ref(false);

const lastGoodDataDate = ref<number | null>(null);
const lastRunningVersion = ref<number>(0);

const rawData = ref<Array<IDataPacket>>([]);

const tempSensors = ref<Array<ITempSensor>>([]);
const executionSteps = ref<Array<IExecutionStep>>([]);
const notifications = ref<Array<INotification>>([]);

const selectedMashSchedule = ref<IMashSchedule | null>(null);

const currentTemps = ref<Array<ITempLog>>([]);

const serverTime = ref<number>();
const serverTimeDelta = ref<number>();
const startDateTime = ref<number>();
const speechVoice = ref<SpeechSynthesisVoice | null>(null);

const stirInterval = ref<Array<number>>([0, 3]);
const stirMax = ref<number>(6);

const dynamicColor = () => {
  const r = Math.floor(Math.random() * 255);
  const g = Math.floor(Math.random() * 255);
  const b = Math.floor(Math.random() * 255);
  return `rgb(${r},${g},${b})`;
};

const beep = async () => {
  // create web audio api context
  const audioCtx = new AudioContext();

  // create Oscillator node
  const oscillator = audioCtx.createOscillator();

  const gainNode = audioCtx.createGain();
  gainNode.gain.value = clientStore.clientSettings.beepVolume; // volume
  gainNode.connect(audioCtx.destination);

  oscillator.type = 'square';
  oscillator.frequency.setValueAtTime(2000, audioCtx.currentTime); // value in hertz

  oscillator.connect(gainNode);

  oscillator.start(audioCtx.currentTime);
  oscillator.stop(audioCtx.currentTime + 0.1); // 100ms beep
};

const speakMessage = async (message: string) => {
  if (clientStore.clientSettings.voiceUri == null) {
    return;
  }

  const synth = window.speechSynthesis;
  // the first time we get the voice and store it in a ref
  if (speechVoice.value == null) {
    const foundVoice = synth.getVoices().find((v) => v.voiceURI === clientStore.clientSettings.voiceUri);
    if (foundVoice !== undefined) {
      speechVoice.value = foundVoice;
    }
  }

  // unable to get a valid voice
  if (speechVoice.value == null) {
    return;
  }

  const ssu = new SpeechSynthesisUtterance(message);
  ssu.voice = speechVoice.value;
  ssu.pitch = 1;
  ssu.rate = clientStore.clientSettings.speechRate;
  ssu.volume = clientStore.clientSettings.speechVolume;
  synth.speak(ssu);
};

const showNotificaton = async (notification: INotification, alert: boolean) => {
  notificationDialogTitle.value = notification.name;
  notificationDialogText.value = notification.message.replaceAll('\n', '<br/>');
  notificationDialog.value = true;

  if (alert && clientStore.clientSettings.beepEnabled) {
    beep();
  }

  if (alert && clientStore.clientSettings.speechEnabled) {
    // when beep and speech add a small pauze
    if (clientStore.clientSettings.beepEnabled) {
      setTimeout(() => {
        speakMessage(notification.message);
      }, 1500);
    } else {
      speakMessage(notification.message);
    }
  }
};

const chartAnnotations = computed(() => {
  // wait for chartjs init
  if (!chartInitDone.value) {
    return null;
  }

  let currentNotifications: Array<INotification> = [];

  // when we are running notifications that come from schedule api call
  if (executionSteps.value != null && executionSteps.value.length > 0) {
    currentNotifications = [...notifications.value];
  } else if (selectedMashSchedule.value !== null && selectedMashSchedule.value.steps !== null && startDateTime.value != null) {
    // when we have no steps and we are idle we can compute notifications from the schedule settings
    if (status.value === 'Idle') {
      const scheduleNotifications = [...selectedMashSchedule.value.notifications];

      currentNotifications = scheduleNotifications.map((notification) => {
        let notificationTime = startDateTime.value!;
        notificationTime += notification.timeFromStart * 60;

        const newNotification = { ...notification };
        newNotification.timePoint = notificationTime;
        return newNotification;
      });
    }
  }

  const annotationData: Array<any> = [];

  currentNotifications.forEach((notification) => {
    const notificationTime = notification.timePoint * 1000;
    const notificationPoint = {
      type: 'line',
      xMin: notificationTime,
      xMax: notificationTime,
      borderColor: 'rgb(255, 99, 132)',
      borderWidth: 2,
      label: {
        content: notification.name,
        drawTime: 'afterDatasetsDraw',
        display: true,
        yAdjust: -110,
        position: 'top',
      },
      click(context: any, event: any) {
        showNotificaton(notification, false);
      },
    };
    annotationData.push(notificationPoint);
  });

  // sync remote and local time
  if(serverTimeDelta.value != null){
    annotationData.forEach((dataset) => {
      dataset.xMin+= serverTimeDelta.value;
      dataset.xMax+= serverTimeDelta.value;
    });
  }
  return annotationData;
});

const chartData = computed(() => {
  // wait for chartjs init
  if (!chartInitDone.value) {
    return null;
  }
  let scheduleData: Array<any> = [];

  if (executionSteps.value != null && executionSteps.value.length > 0) {
    scheduleData = executionSteps.value.map((step: any) => ({
      x: step.time * 1000,
      y: step.temperature,
    }));
  } else if (selectedMashSchedule.value !== null && selectedMashSchedule.value.steps !== null && startDateTime.value != null) {
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
      x: serverTime.value! * 1000,
      y: temperature.value,
    });
  }

  let datasets = [
    {
      label: `${t('control.avg')} ${appStore.tempUnit}`,
      backgroundColor: 'rgba(255, 255, 255, 0.7)',
      borderColor: 'rgba(255, 255, 255, 0.9)',
      lineTension: 0,
      fill: false,
      xAxisID: 'xAxis',
      yAxisID: 'yAxis',
      data: realData,
    },
    {
      label: `${t('control.target')} ${appStore.tempUnit}`,
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

  // sync remote and local time
  if(serverTimeDelta.value != null){
    datasets.forEach((dataset) => {
      dataset.data.forEach((item) => {
        item.x += serverTimeDelta.value;
      })
    });
  }

  return {
    labels: [],
    datasets,
  };
});

const clearAllNotificationTimeouts = () => {
  notificationTimeouts.value.forEach((timeOutId) => {
    window.clearTimeout(timeOutId);
  });

  notificationTimeouts.value = [];
};

const setNotifications = (newNotifications: Array<INotification>) => {
  clearAllNotificationTimeouts();

  const timeoutIds: Array<number> = [];

  //not done, or fist start
  newNotifications.filter((n) => n.done === false || notificationFirstStart.value == true).forEach((notification) => {
    const timeTill = (notification.timePoint-serverTime.value!)*1000;
    const timeoutId = window.setTimeout(() => {
      showNotificaton(notification, true);
    }, timeTill);
    timeoutIds.push(timeoutId);
  });

  notificationTimeouts.value = timeoutIds;
  notificationFirstStart.value = false;

  notifications.value = newNotifications;
};

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
  setServerTime(apiResult.serverTime);
  setNotifications(apiResult.data.notifications as Array<INotification>);

  lastRunningVersion.value = apiResult.data.version;
};

const setServerTime= (value: number) => {
  if(value != null){
    serverTimeDelta.value =  Date.now() - (value*1000);
    serverTime.value = value;
  }
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
  setServerTime(apiResult.serverTime);
  status.value = apiResult.data.status;
  stirStatus.value = apiResult.data.stirStatus;
  temperature.value = apiResult.data.temp;
  outputPercent.value = apiResult.data.output;
  
  if(!manualOverrideOutputChanged.value){
    manualOverrideOutput.value = apiResult.data.manualOverrideOutput;  
  }
  
  if(!targetTemperatureChanged.value){
    targetTemperature.value = apiResult.data.targetTemp;
  }

  lastGoodDataDate.value = apiResult.data.lastLogDateTime;
  inOverTime.value = apiResult.data.inOverTime;
  const serverRunningVersion = apiResult.data.runningVersion;

  // notifications move with overtime and will be re-added when it is done
  if (inOverTime.value) {
    clearAllNotificationTimeouts();
  }

  if (status.value === 'Running' && lastRunningVersion.value !== serverRunningVersion) {
    // the schedule has changed, we need to update
    getRunningSchedule();
  }

  const tempData = [...rawData.value, ...apiResult.data.tempLog];

  // sort data, chartjs seems todo weird things otherwise
  tempData.sort((a, b) => a.time - b.time);

  rawData.value = tempData;

  // if there are more then 1 sensor we also get the raw data per sensor (whitout history)
  const timestampSeconds = serverTime.value!;

  if (apiResult.data.temps !== null) {
    apiResult.data.temps.forEach((t: any) => {
      // find record in templog and add
      const foundRecord = currentTemps.value.find((ct: any) => ct.sensor === t.sensor);
      if (foundRecord === undefined) {
        const newRecord: ITempLog = {
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
  if (targetTemperature.value === undefined) {
    return;
  }

  // for some reason value is still a string while ref defined as number, bug in vue?
  const forceInt = parseInt(targetTemperature.value?.toString(), 10);

  const requestData = {
    command: 'SetTemp',
    data: {
      targetTemp: forceInt,
    },
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
  targetTemperatureChanged.value = false;
};

const changeOverrideOutput = () => {
  if (manualOverrideOutput.value === null) {
    return;
  }

  const forceInt = parseInt(manualOverrideOutput.value.toString(), 10);

  const requestData = {
    command: 'SetOverrideOutput',
    data: {
      output: forceInt,
    },
  };

  webConn?.doPostRequest(requestData);
  // todo capture error

  manualOverrideOutputChanged.value = false;
};

const setStartDateNow = () => {
  startDateTime.value = serverTime.value;
};

const start = async () => {
  const requestData = {
    command: 'Start',
    data: {
      selectedMashSchedule: null as string | null,
    },
  };

  // reset all our data so we can start over
  currentTemps.value = [];
  executionSteps.value = [];
  rawData.value = [];
  notificationFirstStart.value = true;
  setStartDateNow();

  if (selectedMashSchedule.value != null) {
    requestData.data.selectedMashSchedule = selectedMashSchedule.value?.name;
  }

  await webConn?.doPostRequest(requestData);
  // todo capture error
  // reset out running version so we can definitly get the new schedule
  lastRunningVersion.value = 0;
  getData();
};

const stop = async () => {
  const requestData = {
    command: 'Stop',
    data: null,
  };

  clearAllNotificationTimeouts();

  webConn?.doPostRequest(requestData);
  // todo capture error
  getData();
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
  getData();
};

const stopStir = async () => {
  const requestData = {
    command: 'StopStir',
    data: null,
  };

  webConn?.doPostRequest(requestData);
  // todo capture error
  getData();
};


watch(selectedMashSchedule, () => {
  // reset all our data so we can start over
  currentTemps.value = [];
  executionSteps.value = [];
  rawData.value = [];
  setStartDateNow();
});

const initChart = () => {
  ChartJS.register(Title, Tooltip, Legend, PointElement, LineElement, TimeScale, LinearScale, CategoryScale, Filler, annotationPlugin);
  chartInitDone.value = true;
};

const chartOptions = computed<any>(() => {
  let suggestedMin = 60;
  let suggestedMax = 105;
  // ajust min max when farenheit
  if (appStore.temperatureScale === TemperatureScale.Fahrenheit) {
    suggestedMin = 140;
    suggestedMax = 220;
  }

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false, // Disable all animations, does weird things when adding data
    scales: {
      xAxis: {
        title: {
          display: true,
          text: t('control.time'),
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
          text: appStore.tempUnit,
          color: '#ffffff',
        },
        type: 'linear',
        suggestedMin,
        suggestedMax,
        ticks: {
          color: '#ffffff',
        },
        grid: { color: '#bdbdbc' },
      },
    },
    plugins: {
      annotation: {
        annotations: chartAnnotations.value,
      },
    },
  };

  return options;
});

onMounted(() => {
  // atm only used to render te schedule at the current time
  setStartDateNow();

  intervalId.value = setInterval(() => {
    getData();
  }, 3000);

  initChart();
});

onBeforeUnmount(() => {
  clearAllNotificationTimeouts();
  clearInterval(intervalId.value);
});

</script>

<template>
  <v-dialog v-model="notificationDialog" max-width="500px">
    <v-card>
      <v-toolbar density="compact" color="dialog-header">
        <v-toolbar-title>{{ notificationDialogTitle }}</v-toolbar-title>
      </v-toolbar>

      <v-card-text v-html="notificationDialogText" />

      <v-card-actions>
        <v-spacer />
        <v-btn color="blue-darken-1" variant="text" @click="notificationDialog = false">
          Close
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <v-container class="spacing-playground pa-6" fluid>
    <v-form fast-fail @submit.prevent>
      <v-row style="height: 50vh">
        <Line v-if="chartInitDone && chartData" :options="chartOptions" :data="chartData" />
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="status" readonly :label="$t('control.status')" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="temperature" readonly :label="`${$t('control.temperature')} (${appStore.tempUnit})`" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="outputPercent" readonly :label="`${$t('control.output')} (%)`" />
        </v-col> 
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-select :label="$t('control.mashSchedule')" :readonly="status !== 'Idle'" v-model="selectedMashSchedule" :items="appStore.mashSchedules" item-title="name" :filled="appStore.mashSchedules" :clearable="status === 'Idle'" return-object />
        </v-col>
        <v-col cols="12" md="3">
          <div style="display: flex;align-items: center;">
            <v-text-field v-model="targetTemperature" type="number" :label="`${$t('control.target')} (${appStore.tempUnit})`" @input="targetTemperatureChanged=true" />
            <div style="width: 70px">
              <v-btn v-show="targetTemperatureChanged" size="auto" density="compact" color="success" @click="changeTargetTemp()">
                <v-icon >{{ mdiCheckCircle }}</v-icon>
              </v-btn>
              <v-btn v-show="targetTemperatureChanged" size="auto" density="compact" color="error" @click="targetTemperature = undefined; targetTemperatureChanged=false; getData();">
                <v-icon >{{ mdiCloseCircle }}</v-icon>
              </v-btn>
            </div>              
          </div>                    
        </v-col>
        <v-col cols="12" md="3">
          <div style="display: flex;align-items: center;">
            <v-text-field v-model.number="manualOverrideOutput" type="number" :label="$t('control.override_output')" @input="manualOverrideOutputChanged=true" />
            <div style="width: 70px">
              <v-btn v-show="manualOverrideOutputChanged" size="auto" density="compact" color="success" @click="changeOverrideOutput()">
                <v-icon >{{ mdiCheckCircle }}</v-icon>
              </v-btn>
              <v-btn v-show="manualOverrideOutputChanged" size="auto" density="compact" color="error" @click="manualOverrideOutput = null; manualOverrideOutputChanged=false; getData();">
                <v-icon >{{ mdiCloseCircle }}</v-icon>
              </v-btn>
            </div>
          </div>
        </v-col>

      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-btn v-if="status === 'Idle'" color="success" class="mt-4" block @click="start"> {{ $t('control.start') }} </v-btn>
          <v-btn v-else color="error" class="mt-4" block @click="stop"> {{ $t('control.stop') }} </v-btn>
        </v-col>
      </v-row>
      <div v-if="stirStatus !== 'Disabled'">
        <div class="text-subtitle-2 mt-4 mb-2">{{ $t('control.stir_control') }}</div>
        <v-divider :thickness="7" />
        <v-row>
          <v-col cols="12" md="3">
            <v-text-field v-model="stirStatus" readonly :label="$t('control.status')" />
          </v-col>

          <v-col cols="12" md="">
            <v-range-slider
              v-model="stirInterval"
              :label="$t('control.interval')"
              step="1"
              thumb-label="always"
              :max="stirMax">
              <template v-slot:append>
                <v-text-field
                  v-model.number="stirMax"
                  hide-details
                  single-line
                  type="number"
                  variant="outlined"
                  style="width: 70px"
                  density="compact"
                  :label="$t('control.timespan')" />
              </template>
            </v-range-slider>

          </v-col>
        </v-row>
        <v-row>        
          <v-col cols="12" md="3">
            <v-btn v-if="stirStatus === 'Idle'" color="success" class="mt-4" block @click="startStir"> {{ $t('control.start') }} </v-btn>        
            <v-btn v-else color="error" class="mt-4" block @click="stopStir"> {{ $t('control.stop') }} </v-btn>
          </v-col>
          <v-col cols="12" md="3" />
        </v-row>
        </div>      
    </v-form>
  </v-container>
</template>
