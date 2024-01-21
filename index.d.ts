declare module "node-libpd" {
  /**
   * The `pd` initialization configuration object.
   *
   * @interface PdInitConfig
   * @member `numInputChannels` Number of input channels requested.
   * @member `numOutputChannels` Number of output channels requested.
   * @member `sampleRate` Requested sampleRate.
   * @param `ticks` Number of blocks (ticks) processed by `pd`
   * in one run, a `pd` tick is 64 samples. Be aware that this value will affect /
   * throttle the messages sent to and received by `pd`, i.e. more ticks means less
   * precision in the treatement of the messages. A value of 1 or 2 is generally
   * good enough even in constrained platforms such as the RPi.
   *
   * @default
   * {
   *  numInputChannels: 1,
   *  numOutputChannels: 2,
   *  sampleRate: 4800,
   *  ticks: 1
   * }
   */
  interface PdInitConfig {
    numInputChannels?: number;
    numOutputChannels?: number;
    sampleRate?: number;
    ticks?: number;
  }

  /**
   * Description of a `portaudio` device.
   *
   * @interface PaDeviceDescription
   * @member `structVersion` The portaudio device `struct` version.
   * @member `index` The index of the device in the portaudio list.
   * @member `name` The name of the device.
   * @member `maxInputChannels` The maximum number of imput channels of the device.
   * @member `maxOutputChannels` The maximum number of output channels of the device.
   * @member `defaultLowInputLatency` The default low input latency of the device.
   * @member `defaultLowOutputLatency` The default low output latency of the device.
   * @member `defaultHighInputLatency` The default high input latency of the device.
   * @member `defaultHighOutputLatency` The default high output latency of the device.
   * @member `defaultSampleRate` The default sample rate of the device.
   */
  interface PaDeviceDescription {
    structVersion: number;
    index: number;
    name: string;
    maxInputChannels: number;
    maxOutputChannels: number;
    defaultLowInputLatency: number;
    defaultLowOutputLatency: number;
    defaultHighInputLatency: number;
    defaultHighOutputLatency: number;
    defaultSampleRate: number;
  }

  /**
   * `pd` internal messages list.
   *
   * @example
   * pd.subscribe(PdInternalMessages.Print, (msg: string) => {
   *  console.log(`Pure Data printed '${msg}'`);
   * });
   *
   * pd.unsubscribe(PdInternalMessages.Print);
   */
  enum PdInternalMessages {
    Print = "print",
  }

  type PdCallback = (...args: any[]) => void;

  /**
   * Current audio time in seconds since `init` has been called.
   */
  const currentTime: number;

  /**
   * Configure and initialize `pd` instance. You basically want to do that at the
   * startup of the application as the process is blocking and that it can take
   * a long time to have the audio running.
   *
   * @param { PdInitConfig | undefined } options
   * @param { boolean } computeAudio Optional: enable `pd` audio computation. Default is `true`.
   *
   * @returns { boolean } `true` if `pd` was ssuceesfully initialized, `false` otherwise.
   * See also {@link PdInitConfig}
   */
  function init(options?: PdInitConfig, computeAudio?: boolean): boolean;

  /**
   * Destroy the `pd` instance. You basically want to do that when your program
   * exits to clean things up, be aware that any call to the `pd` instance after
   * calling `destroy` migth throw a SegFault error.
   */
  function destroy(): void;

  /**
   * Enable `pd` audio computation.
   *
   * @param { boolean } compute Optional: tells `pd` to compute audio. Default is `true`.
   */
  function computeAudio(compute?: boolean): void;

  /**
   * Get the audio devices count.
   *
   * @returns { number } The number of audio devices.
   */
  function getDevicesCount(): number;

  /**
   * Get the audio devices descriptions.
   *
   * @returns { Array<PaDeviceDescription> } An `array` of audio devices descriptions.
   * See also {@link PaDeviceDescription}
   */
  function listDevices(): Array<PaDeviceDescription>;

  /**
   * Get the default input device description.
   *
   * @returns { PaDeviceDescription | undefined } The default input device description or `undefined`.
   * See also {@link PaDeviceDescription}
   */
  function getDefaultInputDevice(): PaDeviceDescription | undefined;

  /**
   * Get the default output device description.
   *
   * @returns { PaDeviceDescription | undefined } The default output device description or `undefined`.
   * See also {@link PaDeviceDescription}
   */
  function getDefaultOutputDevice(): PaDeviceDescription | undefined;

  /**
   * Get the audio input devices descriptions.
   *
   * @returns { Array<PaDeviceDescription> } An `array` of audio input devices descriptions.
   * See also {@link PaDeviceDescription}
   */
  function getInputDevices(): Array<PaDeviceDescription>;

  /**
   * Get the audio output devices descriptions.
   *
   * @returns { Array<PaDeviceDescription> } An `array` of output input devices descriptions.
   * See also {@link PaDeviceDescription}
   */
  function getOutputDevices(): Array<PaDeviceDescription>;

  /**
   * Get the device description for a specific index in the portaudio array.
   * Though portaudio and javascript arrays should have the same index for the same object,
   * you should better use the index of the `PaDeviceDescription` object.
   *
   * @returns { PaDeviceDescription | undefined } The device description or `undefined` if the index is out of range.
   * See also {@link PaDeviceDescription}
   */
  function getDeviceAtIndex(index: number): PaDeviceDescription | undefined;

  /**
   * Open a `pd` patch instance. As the same patch can be opened several times,
   * think of it as a kind of poly with a nice API, be careful to use patch.$0
   * in your patches. This signature exists for backward compatibility with `pd` native API.
   *
   * @param  { string } name Filename of the patch.
   * @param { string } pathname Absolute path to the `pd` patches.
   * @returns { Patch } Instance of the patch.
   */
  function openPatch(name: string, pathname: string): Patch;
  /**
   * Open a `pd` patch instance. As the same patch can be opened several times,
   * think of it as a kind of poly with a nice API, be careful to use patch.$0
   * in your patches.
   *
   * @param { string } pathname Absolute path to the `pd` patch
   * @returns { Patch } Instance of the patch.
   * @overload
   */
  function openPatch(pathname: string): Patch;
  function openPatch(...args: string[]): Patch;

  /**
   * Close a `pd` patch instance.
   *
   * @param { Patch } patch The patch to close.
   *
   * @returns { Patch | undefined } A `Patch` instance if the patch has been opened.
   */
  function closePatch(patch: Patch): Patch | undefined;

  /**
   * Add a directory to the `pd` search paths, for loading libraries, etc.
   *
   * @param { string } pathname The path to add.
   */
  function addToSearchPath(pathname: string): void;

  /**
   * Clear the `pd` search path.
   */
  function clearSearchPath(): void;

  /**
   * Send a named message to the `pd` backend.
   *
   * @param { string } channel Name of the corresponding `receive` box in the patch.
   * To avoid conflict a good practice is to prepend the channel name with `patch.$0`.
   * @param { any | undefined } value Payload of the message, the corresponding mapping is
   * made with `pd` types: Number -> float, String -> symbol, Array -> list
   * (all value that are neither Number nor String are ignored), else -> bang.
   * @param { number } time Audio time at which the message should be
   * sent. If null or < currentTime, is sent as fast as possible.
   *
   * @tbc messages are processed at `pd` control rate.
   */
  function send(channel: string, value?: any, time?: number): void;

  /**
   * Subscribe to named events sendtby a `pd` patch.
   *
   * @param { string } channel Channel name corresponding to the `pd` send name.
   * @param { PdCallback } callback Callback to execute when an event is received.
   */
  function subscribe(channel: string, callback: PdCallback): void;

  /**
   * Unsubscribe from named events sent by a `pd` patch.
   *
   * @param { string } channel Channel name corresponding to the `pd` send name.
   * @param { PdCallback | undefined } [callback=null] Callback that should stop receive event.
   *  If null, all callbacks of the channel are removed.
   */
  function unsubscribe(channel: string, callback?: PdCallback): void;

  /**
   * Write values into a `pd` array. Be careful with the size of the `pd` arrays
   * (default to `100`) in your patches.
   *
   * @param { string } name Name of the `pd` array.
   * @param { Float32Array } data `Float32Array` containing the data to be written
   * in the `pd` array.
   * @param { number | undefined } [writeLen=data.length] Length of the data to write.
   * @param { number | undefined } [offset=0] Offset of the data.
   *
   * @returns { boolean } `true` if the operation succeeded, `false` otherwise.
   *
   * @todo
   * Confirm behavior of `writeLen` and `offset` parameters.
   */
  function writeArray(
    name: string,
    data: Float32Array,
    writeLen?: number,
    offset?: number
  ): boolean;

  /**
   * Read values from a `pd` array.
   *
   * @param { string } name Name of the `pd` array.
   * @param { Float32Array } data `Float32Array` to populate from `pd` array values..
   * @param { number | undefined } [readLen=data.length] Length of the data to read.
   * @param { number | undefined } [offset=0] Offset of the data.
   *
   * @returns { boolean } `true` if the operation succeeded, `false` otherwise.
   *
   * @todo
   * Confirm behavior of `writeLen` and `offset` parameters.
   */
  function readArray(
    name: string,
    data: Float32Array,
    readLen?: number,
    offset?: number
  ): boolean;

  /**
   * Fill a `pd` array with a given value.
   *
   * @param { string } name Name of the `pd` array.
   * @param { number } value Value used to fill the `pd` array.
   */
  function clearArray(name: string, value?: number);

  /**
   * Retrieve the size of a `pd` array.
   *
   * @param { string } name The name of the `pd` array.
   *
   * @returns { number } The size of the array.
   */
  function arraySize(name: string): number;

  /**
   * Starts an instance of the `pd` GUI.
   *
   * @param { string } pathname The absolute path to the main folder that contains bin/, tcl/ etc.
   * On macOS it is located in /Applications/Pd-{version}.app/Contents/Resources.
   *
   * @returns { boolean } `true` if the operation was successful, `false` otherwise.
   */
  function startGUI(pathname: string): boolean;

  /**
   * Update and handle any GUI message.
   * You should call this function periodically in order to see the GUI.
   *
   * @see https://github.com/libpd/libpd/pull/132#issuecomment-305504516
   */
  function pollGUI(): void;

  /**
   * Stops `pd` GUI.
   */
  function stopGUI(): void;

  /**
   * Object representing a patch instance.
   */
  interface Patch {
    /**
     * Id of the patch. You should use this value to communicate with a given patch
     * in send and receive channel.
     * @readonly
     */
    readonly $0: number;
    /**
     * Validity of the patch instance. `false` typically means that the given filename
     * does not point to a valid `pd` patch, or that the patch has been closed.
     * @readonly
     */
    readonly isValid: boolean;
    /**
     * Name of the `pd` patch file.
     * @readonly
     */
    readonly filename: string;
    /**
     * Directory of the `pd` patch file.
     * @readonly
     */
    readonly path: string;
  }
}

export type NodePd = typeof import("node-libpd");
