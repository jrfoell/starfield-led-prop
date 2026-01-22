#include <MIDI.h>
#include <Adafruit_NeoPXL8.h>

/* BEGIN MIDI Settings */

/**
 * MIDI Notes - Make sure they don't interfere with the metronome
 * https://github.com/jrfoell/visual-metronome/blob/main/visual-metronome.ino
 * If more notes are needed, both could restrict to specific midi channels raather than MIDI_CHANNEL_OMNI
 */
#define NOTE_A0 21
#define NOTE_B0 23
#define NOTE_C1 24
#define NOTE_D1 26
#define NOTE_E1 28
#define NOTE_F1 29
#define NOTE_G1 31
#define NOTE_A1 33
#define NOTE_B1 35
#define NOTE_C2 36
#define NOTE_D2 38
#define NOTE_E2 40
#define NOTE_F2 41
#define NOTE_G2 43
#define NOTE_A2 45
#define NOTE_B2 47

// Override the default MIDI baudrate for FeatherWing
struct CustomBaudRateSettings : public MIDI_NAMESPACE::DefaultSerialSettings {
  static const long BaudRate = 31250;
};

// Leonardo, Due and other USB boards use Serial1 by default.
MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings> serialMIDI(Serial1);
MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>> MIDI((MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>&)serialMIDI);

/* END MIDI Settings */

/* BEGIN NeoPXL8 Settings */

#define NUM_LEDS    30     // NeoPixels PER STRAND, total number is 8X this!
#define COLOR_ORDER NEO_GRB // NeoPixel color format (see Adafruit_NeoPixel)

// Here's a pinout that works with the Feather M4 (not M0) w/NeoPXL8 M4
// FeatherWing in the factory configuration:
int8_t pins[8] = { 13, 12, 11, 10, SCK, 5, 9, 6 };

// Here's the global constructor as explained near the start:
Adafruit_NeoPXL8 leds(NUM_LEDS, pins, COLOR_ORDER);

// For this demo we use a table of 8 hues, one for each strand of pixels:
static uint8_t colors[8][3] = {
  255,   0,   0, // Row 0: Red
  255, 160,   0, // Row 1: Orange
  255, 255,   0, // Row 2: Yellow
    0, 255,   0, // Row 3: Green
    0, 255, 255, // Row 4: Cyan
    0,   0, 255, // Row 5: Blue
  192,   0, 255, // Row 6: Purple
  255,   0, 255  // Row 7: Magenta
};

// Max is 255, 32 is a conservative value
#define BRIGHTNESS 32

/* END NeoPXL8 Settings */

// -----------------------------------------------------------------------------
// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.

    // Try to keep your callbacks short (no delays ect)
    // otherwise it would slow down the loop() and have a bad impact
    // on real-time performance.
    Serial.print("Note on: ");
    Serial.println(pitch);
    
    switch(pitch) {
      case NOTE_A0:
        renderFrame();
        break;

      default:
        break;
    }
  }

// -----------------------------------------------------------------------------

void setup()
{
    // Serial setup
    Serial.begin(115200);
    // Wait for the serial interface to be ready.
    while (!Serial) ;
    Serial.println("MIDI starfield leds");

    // Start NeoPXL8. If begin() returns false, either an invalid pin list
    // was provided, or requested too many pixels for available RAM.
    if (!leds.begin()) {
      // Blink the onboard LED if that happens.
      pinMode(LED_BUILTIN, OUTPUT);
      for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
    }

    // Otherwise, NeoPXL8 is now running, we can continue.
    Serial.print("Setting brightness to ");
    Serial.println(BRIGHTNESS);
    leds.setBrightness(BRIGHTNESS); // Tone it down, NeoPixels are BRIGHT!

    // Cycle all pixels red/green/blue on startup. If you see a different
    // sequence, COLOR_ORDER doesn't match your particular NeoPixel type.
    // If you get a jumble of colors, you're using RGBW NeoPixels with an
    // RGB order. Try different COLOR_ORDER values until code and hardware
    // are in harmony.
    for (uint32_t color = 0xFF0000; color > 0; color >>= 8) {
      leds.fill(color);
      leds.show();
      delay(1000);
    }

    // Light each strip in sequence. This helps verify the mapping of pins to
    // a physical LED installation. If strips flash out of sequence, you can
    // either re-wire, or just change the order of the pins[] array.
    for (int i=0; i<8; i++) {
      if (pins && (pins[i] < 0)) continue; // No pixels on this pin
      leds.fill(0);
      uint32_t color = leds.Color(colors[i][0], colors[i][1], colors[i][2]);
      leds.fill(color, i * NUM_LEDS, NUM_LEDS);
      leds.show();
      delay(300);
    }

    // MIDI Setup
    // Connect the handler function(s) to the library,
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();
    // There is no need to check if there are messages incoming
    // if they are bound to a Callback function.
    // The attached method will be called automatically
    // when the corresponding message has been received.
}

void renderFrame() {
    // render each frame of a repeating animation cycle based on elapsed time:
    uint32_t now = millis(); // Get time once at start of each frame
    for(uint8_t r=0; r<8; r++) { // For each row...
      for(int p=0; p<NUM_LEDS; p++) { // For each pixel of row...
        leds.setPixelColor(r * NUM_LEDS + p, rain(now, r, p));
      }
    }
    leds.show();
}

// Given current time in milliseconds, row number (0-7) and pixel number
// along row (0 - (NUM_LEDS-1)), first calculate brightness (b) of pixel,
// then multiply row color by this and run it through NeoPixel library’s
// gamma-correction table.
uint32_t rain(uint32_t now, uint8_t row, int pixelNum) {
  uint8_t frame = now / 4; // uint8_t rolls over for a 0-255 range
  uint16_t b = 256 - ((frame - row * 32 + pixelNum * 256 / NUM_LEDS) & 0xFF);
  return leds.Color(leds.gamma8((colors[row][0] * b) >> 8),
                    leds.gamma8((colors[row][1] * b) >> 8),
                    leds.gamma8((colors[row][2] * b) >> 8));
}

