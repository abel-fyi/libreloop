# Palette measurements

Source colors, including composited directional knob hints. Text target: 4.5:1; functional indicators: 3:1. Decorative grid lines and disabled controls stay subdued.

Contrast: [W3C](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html). Perceptual separation: [OKLab](https://bottosson.github.io/posts/oklab/). OKLab distances are design measurements, not an accessibility certification.

## Dark

| Color role | RGB | HSV saturation | HSV value | OKLab lightness | Minimum contrast |
| --- | --- | ---: | ---: | ---: | ---: |
| highlight | `#b39add` | 30.3% | 86.7% | 73.3% | 7.00:1 |
| signal | `#bbff55` | 66.7% | 100.0% | 92.3% | 16.73:1 |
| loop | `#ff44cc` | 73.3% | 100.0% | 70.2% | 6.63:1 |
| pan_left | `#bbff55` | 66.7% | 100.0% | 92.3% | 14.31:1 |
| pan_right | `#ff44cc` | 73.3% | 100.0% | 70.2% | 5.67:1 |
| knob | `#888088` | 5.9% | 53.3% | 60.9% | 4.48:1 |
| note | `#9988aa` | 20.0% | 66.7% | 65.4% | 5.59:1 |
| meter_low | `#bbff55` | 66.7% | 100.0% | 92.3% | 16.73:1 |
| meter_mid | `#ffc130` | 81.2% | 100.0% | 84.5% | 12.32:1 |
| meter_high | `#ff4a64` | 71.0% | 100.0% | 67.4% | 6.12:1 |

Shared lavender fill contrast against controls: **7.65:1** (logo spiral lavender).

pan_left/pan_right OKLab separation: **0.497**.

stereo/mono OKLab separation: **0.497**.

meter_low/meter_mid OKLab separation: **0.168**.

meter_mid/meter_high OKLab separation: **0.271**.

Minimum ordinary label contrast: **5.74:1**; selected/hover label contrast: **8.58:1**; directional hints: **3.62:1**.

Dark colored spectrum minimum contrast: **3.22:1**.


White knob pointer against its violet rim: **16.85:1**.

## Clip palette

24 displayed colors (saturation capped at 25%); minimum title/preview contrast **5.14:1**; minimum adjacent-hue OKLab separation **0.030**.

## Result

All measured targets passed.
