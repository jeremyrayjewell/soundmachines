
const int PIN_KICK  = 2;
const int PIN_SNARE = 3;
const int PIN_HAT   = 4;
const int PIN_AUDIO = 9;
const int PIN_CLAP  = 5;  )

float kPhase = 0.0f, kEnv = 0.0f;   
float sPhase = 0.0f, sEnv = 0.0f;   
float hEnv   = 0.0f;                
float hatHPmem = 0.0f;
float hatLPmem = 0.0f;
float cEnv = 0.0f;
int   cStep = 0;
int   cCount = 0;

void setup() {
  pinMode(PIN_KICK,  INPUT_PULLUP);
  pinMode(PIN_SNARE, INPUT_PULLUP);
  pinMode(PIN_HAT,   INPUT_PULLUP);
  pinMode(PIN_CLAP,  INPUT_PULLUP);
  pinMode(PIN_AUDIO, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13)  | _BV(WGM12) | _BV(CS10);
  ICR1   = 512; // ~31.25 kHz
}

inline float fastNoise() {
  static uint16_t s = 1;
  s ^= s << 7; s ^= s >> 9; s ^= s << 8;
  return ((int16_t)s) / 32768.0f;
}

inline float fastNoiseClap() {
  static uint16_t sC = 12345;       
  sC ^= sC << 7; sC ^= sC >> 9; sC ^= sC << 8;
  return ((int16_t)sC) / 32768.0f;
}

void loop() {
  if (digitalRead(PIN_KICK)  == LOW) { kEnv = 1.0f; kPhase = 0.0f; }
  if (digitalRead(PIN_SNARE) == LOW) { sEnv = 1.0f; sPhase = 0.0f; }
  if (digitalRead(PIN_HAT)   == LOW) { hEnv = 1.0f; }
  if (digitalRead(PIN_CLAP)  == LOW) { cEnv = 1.0f; cStep = 0; cCount = 0; }

  for (int i = 0; i < 50; i++) {
    if (kEnv <= 0.001f && sEnv <= 0.001f && hEnv <= 0.001f && cEnv <= 0.001f) {
      OCR1A = 256;
      continue;
    }

    float mix = 0.0f;

    if (kEnv > 0.001f) {
      float kFreq = 100.0f + 80.0f * kEnv;     
      kPhase += kFreq / 20000.0f; if (kPhase > 1.0f) kPhase -= 1.0f;
      float kSig = sin(6.28318f * kPhase) * kEnv;
      kEnv *= 0.990f;                          
      mix += kSig;
    }

    if (sEnv > 0.001f) {
      sPhase += 180.0f / 20000.0f; if (sPhase > 1.0f) sPhase -= 1.0f;
      float tone  = sin(6.28318f * sPhase);
      float noise = fastNoise();                
      const float nMix = 0.65f;                
      float sSig = ((1.0f - nMix) * tone + nMix * noise) * sEnv;
      sEnv *= 0.960f;                          
      mix += sSig;
    }

    if (hEnv > 0.001f) {
      float n = fastNoise();                    

      const float a_hp = 0.55f;                 
      float hp = n - a_hp * hatHPmem;
      hatHPmem = n;

      const float lpCut = 0.30f;                
      hatLPmem += lpCut * (hp - hatLPmem);

      float hSig   = hatLPmem * hEnv;
      hEnv *= 0.90f;                            
      mix += hSig * 0.80f;                     
    }

    if (cEnv > 0.001f) {
      float n = fastNoiseClap();                
      cCount++;
      if (cCount >= 13) { cCount = 0; cStep++; }
      float gate = (cStep % 4 == 0 || cStep % 4 == 2) ? 1.0f : 0.3f;
      float cSig = n * cEnv * gate;
      mix += cSig * 0.9f;                       

      cEnv *= 0.94f;                            
      if (cStep > 12) cEnv = 0;                 
    }

    if (mix > 1.0f) mix = 1.0f; else if (mix < -1.0f) mix = -1.0f;
    int v = 256 + (int)(mix * 240.0f);
    if (v < 0) v = 0; else if (v > 512) v = 512;
    OCR1A = v;
  }
}
