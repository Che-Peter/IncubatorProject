# IncubatorProject 🐣

## Overview
This repository documents my journey building a **poultry incubator system**, starting from a **30‑egg DC prototype** at the beginning of my Master’s program, and later expanding to a **1,200‑egg scaled system**.  
The project combines **embedded electronics, PCB design, sensor integration, and wireless communication** to solve real‑world challenges in poultry hatching.

---

## Prototype: 30‑Egg DC System
- Designed a **DC‑based incubator** with a small internal changeover:
  - One supply charged the battery.
  - The other powered the system directly.
  - battery is resuse when mains is not available
- Early issues:
  - PCB power traces burned out.
  - DHT22 sensor entered non‑responsive states.
  - NRF24L01 module missed signals, causing incomplete records.
- Debugging & fixes:
  - Reinforced PCB lines with cables.
  - Added a reliable DC‑DC converter.
  - Restored sensor and communication stability.
- Results:
  - Started with 25 eggs → 23 hatched.
  - 5 chicks died during growth → **18 healthy chicks sold (weeks old)**.
  - Collected real temperature/humidity data for analysis.

---

## Expansion: 1,200‑Egg System
- Scaled design to handle **industrial capacity**.
- Improved:
  - Power reliability.
  - Sensor stability.
  - Environmental control (temperature, humidity, airflow).
- Integrated monitoring and logging via **Delphi desktop app**.
- Achieved consistent hatch rates at scale.

---

## Technical Highlights
- **Microcontrollers**: Arduino platform.  
- **Sensors**: DHT22 for temperature/humidity.  
- **Wireless**: NRF24L01 for desktop communication.  
- **Software**: Delphi desktop app for monitoring and logging.  
- **PCB Tools**: KiCad / Proteus / Altium Designer.  

---

## Repository Contents
- `/Prototype30Egg` → Arduino code, schematics, data logs.   
- `/Schematics` → PCB and circuit diagrams.  
- `/Data` → CSV logs and plots.  
- `/Images` → Photos of prototypes, scaled incubator, and chicks.  

---

## Lessons Learned
- Difference between a **30‑minute prototype** and a **real‑life product**.  
- Importance of **resilience and debugging** in real conditions.  
- Engineering isn’t just schematics — it’s about solving problems and delivering results.
- Current carrying abilities of different trace widths  

---

## License
This project is licensed under the [MIT License](LICENSE).
