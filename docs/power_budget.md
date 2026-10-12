# Power Budget Analysis

Target: **4+ hours** on a 2000 mAh LiPo at typical use.

## Average Current Draw

| Component | Active (mA) | Duty Cycle | Avg (mA) |
|---|---|---|---|
| ESP32-P4 (inference) | 350 | 15% | 52.5 |
| ESP32-P4 (light sleep) | 5 | 80% | 4.0 |
| ESP32-P4 (deep sleep) | 0.01 | 5% | ~0 |
| Camera (active) | 60 | 15% | 9.0 |
| Camera (off) | 0 | 85% | 0 |
| Audio amp (speaking) | 100 | 10% | 10.0 |
| Audio amp (shutdown) | 0.001 | 90% | ~0 |
| **Total** | — | — | **~76 mA** |

## Estimated Battery Life

```
2000 mAh / 76 mA ≈ 26 hours (theoretical continuous average)
```

Real-world factors reduce this:

- WiFi/BLE config sessions (+20–50 mA when active)
- LDO efficiency (~85%)
- Model loading spikes (~400 mA for 1–2 s at boot)
- Cold weather reduces effective capacity

**Practical target: 4–8 hours** in on-demand mode with ~20 narrations/hour.

## Optimisation Levers

1. **On-demand mode (default)** — zero active draw between button presses
2. **Deep sleep after 5 min idle** — drops to ~10 µA
3. **Camera power gating** — MOSFET cuts 60 mA between captures
4. **Amp shutdown pin** — MAX98357A SD pin saves ~100 mA
5. **Cap inference at 20 tokens** — reduces active CPU time

## Measurement Procedure

1. Insert multimeter in series with battery positive
2. Flash firmware, enter on-demand mode
3. Press button every 30 s for 10 cycles — note peak and average
4. Let device idle 5 min — verify deep sleep entry (< 1 mA)
5. Run continuous mode for 1 hour — log voltage decay
