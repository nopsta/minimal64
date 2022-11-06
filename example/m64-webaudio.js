
var m64_audioBufferSize = 0;
var m64_audioContext    = false;
var m64_audioScriptNode = false;
let m64_audioGainNode   = false;

let m64_volume     = 0.8;
let m64_saveVolume = 0;

let m64_audioNextPlayTime       = 0;
let m64_audioBufferDurationSecs = 0;
let m64_audioSampleFrequency    = 0;

var m64_pushAudioLastTime = 0;
var m64_pauseCounter      = 0;


// push audio should be called as much as possible within reason
// it will check if it's time to schedule another audio buffer to be played
// when an audio buffer starts playing, it will get the next audio buffer from m64 
// and schedule it to start after the current buffer
function m64_pushAudio() {

  if(m64_audioContext.currentTime - m64_pushAudioLastTime > (m64_audioBufferSize * 2) / m64_audioSampleFrequency) {
    m64_pushAudioLastTime = m64_audioContext.currentTime;
    m64_pauseCounter = 10;
    return;
  }
  m64_pushAudioLastTime = m64_audioContext.currentTime;

  if(m64_pauseCounter > 0) {
    m64_pauseCounter--;
    return;
  } else {
    m64_pauseCounter = 0;
  }

  if(m64_audioContext.currentTime + m64_audioBufferDurationSecs * 2 < m64_audioNextPlayTime ) {
    return;
  }

  m64_audioNextPlayTime = m64_audioNextPlayTime + m64_audioBufferDurationSecs;

  var bufferSource = m64_audioContext.createBufferSource();
  bufferSource.addEventListener('ended', (e) => {
    // call m64_pushAudio again when this buffer has finished
    m64_pushAudio();
  });

  var soundBuffer = m64_audioContext.createBuffer(1, m64_audioBufferSize, m64_audioSampleFrequency);
  bufferSource.connect(m64_audioGainNode);

  // get the pointer into the heap for the audio buffer
  var ptr = m64_getAudioBuffer();

  // get a float32 array view of the audio buffer
  var view = new Float32Array(m64.HEAPF32.subarray( (ptr >> 2), (ptr >> 2) + m64_audioBufferSize));

  // set the channel data with the data from the sound buffer
  var channelData = soundBuffer.getChannelData(0);
  channelData.set(view);

  // set the buffer for tthe buffer source and tell it when to play
  bufferSource.buffer = soundBuffer;
  bufferSource.start(m64_audioNextPlayTime);
}


// create an empty buffer 1/4 the size of the audio buffer
// and play it, when it finishes call m64_runTimer again
// not used to output audio, just used so m64_pushAudio() is called more often
function m64_runTimer() {
  m64_pushAudio();

  var bufferSource = m64_audioContext.createBufferSource();
  bufferSource.addEventListener('ended', (e) => {
    m64_runTimer();
  });

  var soundBuffer = m64_audioContext.createBuffer(1, m64_audioBufferSize / 4, m64_audioSampleFrequency);
  bufferSource.connect(m64_audioContext.destination);

  bufferSource.buffer = soundBuffer;

  bufferSource.start(m64_audioContext.currentTime);
}


// set up audio nodes if they're not already set up
function m64_setupAudioNodes() {
  if(m64_audioContext.state != 'running') {
    return;
  }

  if(!m64_audioGainNode) {
    m64_audioGainNode = m64_audioContext.createGain();
    m64_audioGainNode.gain.value = m64_volume;
    m64_audioGainNode.connect(m64_audioContext.destination);

    m64_audioNextPlayTime = m64_audioContext.currentTime + 3 * m64_audioBufferDurationSecs;

    m64_audioInit(m64_audioBufferSize, m64_audioContext.sampleRate);

    // start a timer based on empty audio buffers which will call m64_pushAudio
    m64_runTimer();

    // start another interval timer which will call m64_pushAudio regularly
    setInterval(m64_pushAudio, 4);
  }

}

// call this function to start audio
// should be called in response to a user action so the audio context starts in running state
// or just call it as often as you want to make sure audio has started
function m64_startAudio() {
  var AudioContext = window.AudioContext 
                    || window.webkitAudioContext 
                    || false;

  if (!AudioContext) {
    return;
  }

  if(m64_audioContext && m64_audioContext.state != 'running') {
    // audio context has been created but isnt running
    m64_audioContext.resume();
  }

  try {
    m64_audioBufferSize = 4096;

    if(!m64_audioContext) {
      m64_audioContext = new AudioContext();
      
      m64_audioSampleFrequency = m64_audioContext.sampleRate;
      m64_audioBufferDurationSecs = m64_audioBufferSize / m64_audioSampleFrequency;

      m64_audioContext.onstatechange = () => {
        m64_setupAudioNodes();
      }
    }

    if(m64_audioContext.state != 'running') {
      // try to resume audio context if its not running
      m64_audioContext.resume();
    }

  } catch(err) {
    console.log("ERRR");
    console.log(err);
  }
}