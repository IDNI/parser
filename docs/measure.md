[back to index](../README.md#classes-and-structs)

# measure

`measure` is a structure capable of measuring elapsed time. Values printed and returned are in milliseconds (ms).

## constructor

```
measure(const std::string& label, bool start_measure = false, bool silent = false);
```

Creates a measuring object labeled with a `label`.

If `start_measure` is true it starts measuring when instantiated (constructor calls `start()`).

If `silent` is true it prints no measurement info but `stop()` and `pause()` still returns elapsed time in ms.


## methods

### void start();

Starts measuring.

### double pause();

Pauses measuring and returns elapsed time from start in ms.

### double unpause();

Unpause measuring.

### double stop();

Stops measuring, if `silent` is not `true` it prints measured info and returns elapsed time in ms.

`stop()` is called when measur struct is destructed.
