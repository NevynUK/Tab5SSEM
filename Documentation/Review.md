# Code Review — Tab5SSEM

Date: 11 April 2026

---

## Code Clarity

### 6. `Display` mutex acquired without a scope guard
Every public `Display` method has matching `xSemaphoreTake` / `xSemaphoreGive`
calls written by hand.  A small RAII scope guard removes the risk of missing
a `Give` on any future early-return path and makes locking intent clearer.

---

## Performance

### 7. `esp_timer_get_time()` called every instruction at maximum speed
This is a hardware register read on every iteration of the tight inner loop
and is likely the dominant overhead at `SpeedSetting::Maximum`.  Throttle it
so the call only occurs when `instructionCount` crosses a cheap modulo
threshold:

```cpp
if ((instructionCount & 0xFFU) == 0)
{
    const int64_t nowUs = esp_timer_get_time();
    ...
}
```

---

## Modern C++ Standards

### 12. Raw `FILE*` without RAII in `ReadSdCardFileContents`
If any future early-return path is added before `fclose`, the file handle leaks.
Wrap it in a `unique_ptr` with a custom deleter:

```cpp
auto fileDeleter = [](FILE *f) { if (f) fclose(f); };
std::unique_ptr<FILE, decltype(fileDeleter)> file(fopen(fullPath.c_str(), "r"), fileDeleter);
if (!file) { ... return lines; }
```

### 14. Unchecked queue and mutex creation in `Display::Run()`
`xQueueCreate` and `xSemaphoreCreateMutex` both return `nullptr` on allocation
failure.  On an embedded target with limited heap these can legitimately fail.
Both return values should be validated with `configASSERT` or an
`ESP_ERROR_CHECK`-equivalent immediately after creation.
