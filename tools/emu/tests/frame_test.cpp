// What reaches the panel, and when.
//
// A flush pushes the whole ~434 KB framebuffer over QSPI. loop() used to draw
// and push ten times a second whatever the panel was doing -- including with
// the screen switched OFF, where not one of those frames could be seen.
// FrameCanvas (the sketch's canvas) decides whether a frame is pushed; this
// test drives the real loop() and counts what actually went out, which the
// stub canvas records in `pushes` and `pushBrightness`.
//
// flush_test answers a different question -- does every render path ASK for a
// flush -- and keeps answering it; this one is about the pushes themselves.
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "TouchDrvCSTXXX.hpp"
#include "Preferences.h"
#include "pet.h"
#include "party.h"
#include <cstdio>
#include <chrono>
#include <thread>
uint32_t g_seed=4242; FakeSerial Serial; FakeESP ESP; FakeWire Wire;
volatile int g_touchX=0,g_touchY=0; volatile bool g_touchDown=false;
void FakeESP::restart(){exit(0);}
int FakeSerial::available(){return 0;}
String FakeSerial::readStringUntil(char){return String("");}
void setup(); void loop(); void render();
uint8_t uiCurrentScreen();
extern Pet pet;
extern Arduino_Canvas *gfx;
extern Arduino_CO5300 *panel;
extern bool screenOff, menuOpen, sackOpen, partyOpen;
extern uint8_t dimStage;
extern uint32_t lastInteract;
extern uint32_t gFlushCalls, gFramePushes;
extern uint32_t gCrumbScreen;
void startSack();

static int bad=0;
static void ck(bool ok,const char*w){printf("%s  %s\n",ok?"PASS":"FAIL",w); if(!ok)bad++;}

// real time, because loop() paces itself off millis()
static void pumpMs(int ms){
  auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (std::chrono::steady_clock::now() < end) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    loop();
  }
}
static void touch(int x,int y){
  g_touchX=x; g_touchY=y; g_touchDown=true; emuFireInterrupt(); pumpMs(40);
  g_touchDown=false; emuFireInterrupt(); pumpMs(40);
}

int main(){
  setup();
  if (pet.awaitingStarter()) pet.chooseStarter(4);
  if (pet.isEgg()) pet.dbgHatchAs(6, false);
  while (pet.hasLearnOffer()) pet.declineLearn();
  pet.sleeping = false;
  lastInteract = millis();
  pumpMs(300);

  // --- 1. screen on: frames are drawn ------------------------------------
  uint32_t c0 = gFlushCalls;
  pumpMs(500);
  printf("     lit, 500 ms: %u flushes asked for\n", gFlushCalls - c0);
  ck(gFlushCalls - c0 >= 3, "with the screen on, render() runs at its normal rate");

  // --- 2. screen off: nothing drawn, nothing pushed, crumb still kept ------
  screenOff = true;
  pumpMs(150);                      // the pass that notices it went dark
  c0 = gFlushCalls;
  uint32_t p0 = gfx->pushes;
  menuOpen = true;                  // the screen changes while dark
  pumpMs(800);
  printf("     dark, 800 ms: %u flushes asked for, %u pushed\n",
         gFlushCalls - c0, gfx->pushes - p0);
  ck(gFlushCalls == c0, "with the screen off, nothing is drawn at all");
  ck(gfx->pushes == p0, "and nothing is pushed to the panel");
  ck(gCrumbScreen == uiCurrentScreen(),
     "but the crash breadcrumb still follows the screen that is up");
  menuOpen = false;

  // A render path that IS game logic keeps running in the dark: the bag counts
  // and ends on its own clock inside renderSack(). Drawn, never pushed.
  startSack();
  c0 = gFlushCalls; p0 = gfx->pushes;
  pumpMs(500);
  printf("     dark minigame, 500 ms: %u flushes asked for, %u pushed\n",
         gFlushCalls - c0, gfx->pushes - p0);
  ck(gFlushCalls - c0 >= 3, "a minigame keeps stepping with the screen off");
  ck(gfx->pushes == p0, "without a single frame pushed");
  sackOpen = false;

  // --- 3. waking: a fresh frame is pushed BEFORE the panel is lit ----------
  pumpMs(150);
  p0 = gfx->pushes;
  ck(panel->brightness == 0, "the panel really is dark before the wake");
  // the real path: a touch while dark wakes it (handleTouch clears screenOff)
  std::this_thread::sleep_for(std::chrono::milliseconds(25));   // past the 20 ms touch poll gate
  g_touchX=233; g_touchY=200; g_touchDown=true; emuFireInterrupt();
  loop();                           // ONE pass: the wake must happen inside it
  ck(!screenOff, "the touch woke the screen");
  ck(gfx->pushes > p0, "the wake pushes a frame in the same loop pass");
  ck(gfx->pushBrightness == 0,
     "and pushes it while the panel is still dark, so a stale frame is never lit");
  ck(panel->brightness > 0, "and then lights the panel");
  g_touchDown=false; emuFireInterrupt(); pumpMs(100);

  // --- 4. dimStage 2: about 2 fps -----------------------------------------
  lastInteract = millis() - 400000;     // 400 s idle
  pumpMs(150);
  ck(dimStage == 2, "400 s without a touch reaches dimStage 2");
  c0 = gFlushCalls;
  pumpMs(2000);
  uint32_t dimN = gFlushCalls - c0;
  printf("     dimStage 2, 2000 ms: %u frames\n", dimN);
  ck(dimN >= 3 && dimN <= 6, "dimStage 2 draws about 2 frames a second");
  // and a touch brings the normal rate straight back
  touch(233, 60);
  c0 = gFlushCalls;
  pumpMs(500);
  ck(dimStage == 0 && gFlushCalls - c0 >= 3, "a touch restores the full rate at once");

  printf("%s\n", bad?"FAILURES":"all good");
  return bad?1:0;
}
