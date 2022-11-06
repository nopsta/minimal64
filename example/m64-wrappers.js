var m64 = M64({});

/******* WRAPPERS FOR M64 FUNCTIONS ********/

// functions to be used if you want to provide your own ROM files
// by default, minimal64 uses its own custom kernal
// these functions should be called before m64_init()
// m64_setCharacterROM(romData, romDataLength)
// m64_setKernalROM(romData, romDataLength)
// m64_setBASICROM(romData, romDataLength)
// romData       : A Uint8Array containing the character rom 
// romDataLength : The length of the character ROM data array
var m64_setCharacterROM = m64.cwrap('m64_setCharacterROM', null, ['array','number']);
var m64_setKernalROM = m64.cwrap('m64_setKernalROM', null, ['array','number']);
var m64_setBASICROM = m64.cwrap('m64_setBASICROM', null, ['array','number']);

// m64_init(model, sidModel)
// model    : 0 = NTSC, 1 = PAL
// sidModel : 0 = 6581, 1 = 8580, 2 = 8580 + digiboost
var m64_init = m64.cwrap('m64_init', null, ['number', 'number']);

// m64_audioInit(audioBufferSize, sampleFrequency)
// audioBufferSize : Should be one of 1024, 2048, 4096, 8192
// sampleFrequency : The sample frequency provided by WebAudio
var m64_audioInit = m64.cwrap('m64_audioInit', null, ['number', 'number']);

// m64_injectPrg(prgData, prgDataLength)
// prgData : A Uint8Array containing the prg data
// prg data will be injected into system ram at the location specified by first two bytes
var m64_injectPrg = m64.cwrap('m64_injectPrg', null, ['array','number']);


// m64_injectPrg(prgData, prgDataLength, delay)
// prgData       : A Uint8Array containing the prg data
// prgDataLength : The length of the prgData array
// delay         : Approx number of cycles to delay before running - can be used to add a random element
// this will reset the machine and run it to a state where a prg can be run
// then prg data will be injected into system ram at the location specified by first two bytes
// if using the minimal64 kernal, the function will look for a sys basic command and jump to the address it specifies
// if a kernal has been set with m64_setKernalROM, the function will put 'RUN:' and RETURN into the standard location for the keyboard buffer
var m64_injectAndRunPrg = m64.cwrap('m64_injectAndRunPrg', null, ['array','number','number']);

// m64_loadCartridge(crtData, crtDataLength)
// crtData       : A Uint8Array containing the crt data
// crtDataLength : the length of crtData array
// this will attach a cartridge and restart the machine
// cartridge formats supported: Standard 8kb and 16kb Cartridges, Ocean Type 1, C64GS, Magic Desk
var m64_loadCartridge = m64.cwrap('m64_loadCartridge', null, ['array','number']);

// m64_keyPush(keyCode)
var m64_keyPush = m64.cwrap('m64_keyPush', null, ['number']);

// m64_keyRelease(keyCode)
var m64_keyRelease = m64.cwrap('m64_keyRelease', null, ['number']);

// m64_joystickPush(port, direction)
// port      : 0 for port 1, 1 for port 2
// direction : 1 = up, 2 = down, 4 = left, 8 = right, 16 = fire button
var m64_joystickPush = m64.cwrap('m64_joystickPush', null, ['number', 'number']);

// m64_joystickRelease(port, direction)
// port      : 0 for port 1, 1 for port 2
// direction : 1 = up, 2 = down, 4 = left, 8 = right, 16 = fire button
var m64_joystickRelease = m64.cwrap('m64_joystickRelease', null, ['number', 'number']);

// m64_setSIDModel(model) 
// model : 0 = 6581, 1 = 8580, 2 = 8580 with digiboost
var m64_setSIDModel = m64.cwrap('m64_setSIDModel', null, ['number']);

// m64_reset(runUntilKernalIsReady)
// runUntilKernalIsReady : after reset, run the kernal until it is ready for user input
var m64_reset = m64.cwrap('m64_reset');

// m64_getPixelBuffer()
// returns a pointer to the location of the screen pixels in the heap
// in each group of 4 bytes, pixels are in the order R, G, B, A
var m64_getPixelBuffer = m64.cwrap('m64_getPixelBuffer', 'number');

// m64_getPixelBufferWidth() return the width of the pixel buffer
var m64_getPixelBufferWidth = m64.cwrap('m64_getPixelBufferWidth', 'number');

// m64_getPixelBufferHeight return the height of the pixel buffer
// this is the height of the buffer
// the actual height of pixels to be displayed will depend on the model seleceted (NTSC or PAL)
var m64_getPixelBufferHeight = m64.cwrap('m64_getPixelBufferHeight', 'number');

// m64_setColor(colorIndex, colorABGR)
// colorIndex: the color index to set (0-15)
// colorABGR: the color in ABGR8888 format (Alpha is highest byte, Red is Lowest)
var m64_setColor = m64.cwrap('m64_setColor', null, ['number','number']);

// m64_getAudioBuffer
// returns a poinetr to the location of the audio buffer in the heap
// autio buffer is a float32 array
var m64_getAudioBuffer = m64.cwrap('m64_getAudioBuffer', 'number');

// m64_update(dTime)
// run the m64 for dTime miliseconds, returns 1 if pixelbuffer has been updated, 0 otherwise
var m64_update = m64.cwrap('m64_update','number', ['number']);

