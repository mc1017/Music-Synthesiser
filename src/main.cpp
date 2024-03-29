#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>
#include <Keyboard.h>
#include <CAN_HandShake.h>
#include <Waveform.h>
#include <Display.h>
#include <Drum.h>
#include <vector>

SemaphoreHandle_t keyArrayMutex;
SemaphoreHandle_t minMaxMutex;
SemaphoreHandle_t rotationMutex;
SemaphoreHandle_t currentStepSizeMutex;
SemaphoreHandle_t RX_MessageMutex;
SemaphoreHandle_t CAN_TXSemaphore;
// CAN queue
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;

void sampleISR()
{
    boolean keyboard3 = 0;
    // Calculate the output for each active note
    if (RX_Message2[5] == 2)
    {
        keyboard3 = 1;
    }

    uint8_t activeNotes = 0;
    int32_t Vout = 0;
    for (uint8_t i = 0; i < 3; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            int currentIndex = i * 4 + j;
            uint8_t key = ((keyArray[i] >> j) & 1);
            if (key == 0)
            {
                octave = 0 + knob[1].getRotation();
                waveforms(Vout, activeNotes, currentIndex);
                drum(Vout, activeNotes, currentIndex);
            }
            key = ((keyArray2[i] >> j) & 1);
            if (key == 0)
            {
                octave = 1 + knob[1].getRotation();
                waveforms(Vout, activeNotes, currentIndex);
            }
            if (keyboard3)
            {
                key = ((keyArray3[i] >> j) & 1);
                if (key == 0)
                {
                    octave = 2 + knob[1].getRotation();
                    waveforms(Vout, activeNotes, currentIndex);
                }
            }
        }
    }

    // Avoid Clipping
    if (activeNotes > 1)
    {
        Vout = Vout / activeNotes;
    }
    analogWrite(OUTR_PIN, Vout + 128);
}

void CAN_RX_ISR(void)
{
    uint8_t RX_Message_ISR[8];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR(void)
{
    xSemaphoreGiveFromISR(CAN_TXSemaphore, NULL);
}

// CAN Recieve Task
void Can_RX_Task(void *pvParameters)
{
    uint8_t tmp_RX_Message[8] = {0};

    while (1)
    {
        xQueueReceive(msgInQ, tmp_RX_Message, portMAX_DELAY);
        xSemaphoreTake(RX_MessageMutex, portMAX_DELAY);
        for (int i = 0; i < 8; i++)
        {
            if (tmp_RX_Message[5] == 1)
            {
                RX_Message[i] = tmp_RX_Message[i];
            }
            else
            {
                RX_Message2[i] = tmp_RX_Message[i];
            }
        }
        xSemaphoreGive(RX_MessageMutex);
    }
}

// CAN send Task
void CAN_TX_Task(void *pvParameters)
{
    uint8_t msgOut[8];
    while (1)
    {
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        xSemaphoreTake(CAN_TXSemaphore, portMAX_DELAY);
        CAN_TX(0x111, msgOut);
    }
}

void canBusInitRoutine()
{
    // Initialize CAN bus and disable looping
    uint32_t status = CAN_Init(false);

    CAN_TXSemaphore = xSemaphoreCreateCounting(3, 3);

    // Set filters for CAN signals
    status = setCANFilter(CAN_ID, 0x7ff);

    CAN_RegisterRX_ISR(CAN_RX_ISR);
    CAN_RegisterTX_ISR(CAN_TX_ISR);

    status = CAN_Start();

    msgInQ = xQueueCreate(36, 8);
    msgOutQ = xQueueCreate(36, 8);
}

void scanKeysTask(void *pvParameters)
{
    const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint8_t tmp_RX[8] = {0};
    uint8_t tmp_RX_2[8] = {0};
    uint8_t test_array[5] = {0};
    uint8_t TX_Message[8] = {0};

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (!transmitter && multipleModule)
        {
            xSemaphoreTake(RX_MessageMutex, portMAX_DELAY);
            if (RX_Message[5] == 1)
            {
                for (int i = 0; i < 8; i++)
                {
                    tmp_RX[i] = RX_Message[i];
                }
            }

            for (int i = 0; i < 8; i++)
            {
                tmp_RX_2[i] = RX_Message2[i];
            }

            xSemaphoreGive(RX_MessageMutex);
        }
        else if (transmitter && multipleModule)
        {
            octave = position;
        }
        for (uint8_t i = 0; i < 5; i++)
        {
            setRow(i);
            delayMicroseconds(3);
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
            keyArray[i] = readCols();
            TX_Message[i] = keyArray[i];
            if (i < 3)
            {
                if (!transmitter && multipleModule)
                {
                    keyArray[i] = keyArray[i];
                    keyArray2[i] = tmp_RX[i];
                    keyArray3[i] = tmp_RX_2[i];
                }
                else
                {
                    keyArray[i] = keyArray[i];
                }
            }

            test_array[i] = keyArray[i];

            uint8_t currentA0 = (keyArray[i] & 0b00000100) >> 2;
            uint8_t currentB0 = (keyArray[i] & 0b00001000) >> 3;
            uint8_t currentA1 = keyArray[i] & 0b00000001;
            uint8_t currentB1 = (keyArray[i] & 0b00000010) >> 1;
            uint8_t currentA2 = (keyArray[i] & 0b00000100) >> 2;
            uint8_t currentB2 = (keyArray[i] & 0b00001000) >> 3;
            uint8_t currentA3 = keyArray[i] & 0b00000001;
            uint8_t currentB3 = (keyArray[i] & 0b00000010) >> 1;
            xSemaphoreTake(rotationMutex, portMAX_DELAY);
            switch (i)
            {
            case 3:
                knob[3].updateRotation(currentA3, currentB3);

                knob[2].updateRotation(currentA2, currentB2);

                break;
            case 4:
                knob[1].updateRotation(currentA1, currentB1);

                knob[0].updateRotation(currentA0, currentB0);

                break;

            default:
                break;
            }
            xSemaphoreGive(rotationMutex);
            xSemaphoreGive(keyArrayMutex);
        }
        TX_Message[5] = position;

        displayKeys(keyArray[0], keyArray[1], keyArray[2]);

        if (transmitter && multipleModule)
        {
            xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        }
    }
}

void updateDisplayTask(void *pvParameters)
{
    int highestBit = -1;
    const uint32_t notes[] = {};
    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_t0_11_tf);

        const uint32_t *stepSizes = stepSizeList[octave];
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        highestBit = highest_unset_bit(keyArray);
        xSemaphoreGive(keyArrayMutex);
        __atomic_store_n(&currentStepSize, (highestBit < 0) ? 0 : stepSizes[highestBit], __ATOMIC_SEQ_CST);

        if (transmitter && multipleModule)
        {
            displayTransmitter();
        }
        else
        {
            // Display menu elements
            displayVolume(knob[3].getRotation());
            displayMode(knob[2].getRotation());
            displayOctave(knob[1].getRotation(), false);
            displayWaveform(knob[0].getRotation());
        }

        displayTXRX(transmitter, multipleModule, position);

        // Draw small wave symbol
        u8g2.setFont(u8g2_font_open_iconic_play_1x_t);
        u8g2.drawGlyph(7, 8, 64);

        u8g2.sendBuffer();

        // Toggle LED
        digitalToggle(LED_BUILTIN);
    }
}

void setup()
{

    // Set pin directions
    pinMode(RA0_PIN, OUTPUT);
    pinMode(RA1_PIN, OUTPUT);
    pinMode(RA2_PIN, OUTPUT);
    pinMode(REN_PIN, OUTPUT);
    pinMode(OUT_PIN, OUTPUT);
    pinMode(OUTL_PIN, OUTPUT);
    pinMode(OUTR_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(C0_PIN, INPUT);
    pinMode(C1_PIN, INPUT);
    pinMode(C2_PIN, INPUT);
    pinMode(C3_PIN, INPUT);
    pinMode(JOYX_PIN, INPUT);
    pinMode(JOYY_PIN, INPUT);

    deviceID = HAL_GetUIDw0();

    // Initialise display
    setOutMuxBit(DRST_BIT, LOW); // Assert display logic reset
    delayMicroseconds(2);
    setOutMuxBit(DRST_BIT, HIGH); // Release display logic reset
    u8g2.begin();
    setOutMuxBit(DEN_BIT, HIGH); // Enable display power supply
    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);
    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();

    canBusInitRoutine();

    multipleModule = handshakeRoutine(position);

    // Resend Handshake End signal to ensure all modules exit the loop
    delay(100);
    sendCAN_HSEnd();

    setOutPin();

    minMaxMutex = xSemaphoreCreateMutex();
    keyArrayMutex = xSemaphoreCreateMutex();
    rotationMutex = xSemaphoreCreateMutex();
    currentStepSizeMutex = xSemaphoreCreateMutex();
    RX_MessageMutex = xSemaphoreCreateMutex();

    if (position == 0)
        transmitter = false;
    else
        transmitter = true;
    TaskHandle_t scanKeysHandle = NULL;
    xTaskCreate(
        scanKeysTask, /* Function that implements the task */
        "scanKeys",   /* Text name for the task */
        500,          /* Stack size in words, not bytes */
        NULL,         /* Parameter passed into the task */
        2,            /* Task priority */
        &scanKeysHandle);

    TaskHandle_t updateDisplayHandle = NULL;
    xTaskCreate(
        updateDisplayTask, /* Function that implements the task */
        "updateDisplay",   /* Text name for the task */
        500,               /* Stack size in words, not bytes */
        NULL,              /* Parameter passed into the task */
        1,                 /* Task priority */
        &updateDisplayHandle);
    if (multipleModule && transmitter)
    {
        TaskHandle_t CanSendHandle = NULL;
        xTaskCreate(
            CAN_TX_Task, /* Function that implements the task */
            "CAN_TX",    /* Text name for the task */
            200,         /* Stack size in words, not bytes */
            NULL,        /* Parameter passed into the task */
            3,           /* Task priority */
            &CanSendHandle);
    }
    else if (multipleModule && !transmitter)
    {
        TaskHandle_t CanReceiveHandle = NULL;
        xTaskCreate(
            Can_RX_Task, /* Function that implements the task */
            "Can_RX",    /* Text name for the task */
            200,         /* Stack size in words, not bytes */
            NULL,        /* Parameter passed into the task */
            3,           /* Task priority */
            &CanReceiveHandle);
    }
    vTaskStartScheduler();
}

void loop()
{
}