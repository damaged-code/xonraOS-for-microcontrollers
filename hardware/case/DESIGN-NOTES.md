# Case design notes (why it looks the way it does)

The case was redesigned after reviewing published material on premium product and enclosure design
(industrial-design guides, design reviews of premium handhelds, colour-material-finish guidance and 3D-printing
enclosure guides). What we took from it, and what it changed:

| Finding | Change in the case |
|---|---|
| Good design is restrained: no part or detail that does not serve a purpose (Dieter Rams; the Playdate is often described this way). | Removed the faceted triangle band from the walls. One clean shape. |
| The seam between two halves is hard to hide; premium products make it a deliberate feature. A small chamfer (about 0.2–0.4 mm) on both halves, or an overlapping "shadow line", hides misalignment. | A 0.5 × 0.8 mm recessed step at the top of the base (shadow line) and a 0.4 mm edge on the lid. The lid seems to float on the base. |
| Fillets read as softer and more premium than plain chamfers; blended, continuous edges are a quality cue. | Top and bottom edges are rounded (1.4 mm and 1.0 mm fillets), corners 4.5 mm. |
| Colour, material and finish: a tone-on-tone body looks sleek and monolithic; use one accent colour to point at the key feature. Matte/textured surfaces hide 3D-print layer lines. | One matte graphite colour for lid and base, a glossy black screen panel, and a single light-grey accent on the main button. |
| Signature control: the Playdate crank and Rabbit r1 scroll wheel make one control the identity of the device. | The side scroll wheel sits in a shallow thumb scoop so it is easy to find and roll. |
| Ergonomics: controls in the lower-centre thumb zone, targets of at least about 9 mm; give a clear main control; recessed buttons that are hard to find get criticised (Analogue Pocket reviews). | Buttons sit below the screen in the thumb zone, raised 1 mm. The main button is 16 mm, the second 12 mm. |
| FDM enclosures: walls of at least 2 mm, about 0.5 mm clearance between printed parts and components, lip-and-groove for alignment with screws doing the holding. | Walls 2.0 mm, 0.6 mm clearance around button caps, lid lip in the base. Six ramped snap-fit clips hold the lid (no screws, smooth underside); pins on the posts locate the board. |

## Printing for a premium finish

- Matte graphite PLA or PETG for lid and base; print the lid top-face-down on a **textured PEI** sheet for an even matte top.
- Glossy black screen panel: print it top-face-down on a smooth PEI or glass bed.
- Main button cap in light grey, second cap in black.
- Optional: light sanding (400 → 800 grit) of the edges, or a matte clear coat.

## Sources

- MIT Toobers, "Sweat the details": https://toylab.mit.edu/toobers/cad/sweat-the-details
- OpenAT design guide, shadow lines: https://makersmakingchange.github.io/OpenAT_Design_Guide/Design_Elements_For_3D_Printed_Parts/Shadow-Lines.html
- FirstMold, fillets and chamfers: https://firstmold.com/tips/fillets-and-chamfers/
- AttackShark, chamfers and premium machining cues: https://attackshark.com/blogs/knowledges/chamfers-metal-keyboards-ergonomic-cnc-machining-guide
- Puxiang, chamfer styles in consumer electronics: https://en.puxiang.com/galleries/f36b1334e2213c59e566773d2155d99f
- JLCPCB, rounded corners: https://jlcpcb.com/blog/rounded-corners-pcb-design
- Promwad, milled enclosure design: https://promwad.com/news/milled-enclosure-design-guide
- Sessions College, "Delicious design: Playdate": https://www.sessions.edu/notes-on-design/delicious-design-playdate/
- Wallpaper, Playdate: https://wallpaper.com/technology/panic-playdate-console-instant-cult-gaming-object
- Domus, Playdate: https://domusweb.it/en/design/gallery/2022/06/24/playdate-a-game-boy-designed-for-the-netflix-era.html
- Fast Company, Rabbit r1 design: https://fastcompany.com/91013196/how-design-drove-10m-in-pre-orders-for-rabbit-r1-ai-hardware
- Yanko Design, Rabbit r1: https://www.yankodesign.com/2024/01/12/rabbit-r1-co-designed-with-teenage-engineering-is-a-cute-pocket-ai-assistant/
- MoMA, iPod: https://www.moma.org/collection/works/89465
- Chicago Design Stories, roots of the iPod classic design: https://www.chicagodesignstories.com/post/the-roots-of-the-ipod-classic-s-design-language
- Design Museum, Dieter Rams: https://designmuseum.org/designers/dieter-rams
- Rams Foundation: https://rams-foundation.org/?p=7213
- Whipsaw, colour material finish: https://www.whipsaw.com/latest/the-strategy-of-color-material-finish
- Covestro, CMF trends: https://solutions.covestro.com/ja/highlights/articles/cases/2024/cmf-design-trends-2025
- CAD Crowd, psychology of perceived quality: https://www.cadcrowd.com/blog/?p=50494
- UL Prospector, colour material finish: https://ulprospector.ul.com/18834/pe-leveraging-the-impact-of-color-material-finish-in-product-design/
- Mobiforge, designing for thumbs: https://mobiforge.com/design-development/designing-touch-thumb-and-finger-sized-design
- Microsoft Research, target size for one-handed thumb use: https://www.microsoft.com/en-us/research/?p=155150
- Retro Dodo, Analogue Pocket review: https://retrododo.com/analogue-pocket-review/
- Creative Bloq, Analogue Pocket review: https://www.creativebloq.com/reviews/analogue-pocket
- Hubs, enclosure design for 3D printing: https://www.hubs.com/knowledge-base/enclosure-design-3d-printing-step-step-guide/
- Hackaday, simple tips for better 3D printed enclosures: https://hackaday.com/2020/11/16/simple-tips-for-better-3d-printed-enclosures
- Sculpteo, 3D printed enclosure guidelines: https://www.sculpteo.com/blog/2017/06/14/3d-printed-enclosures-learn-the-basic-design-guidelines/
- FacFox, enclosure design guide: https://facfox.com/docs/kb/enclosure-design-guide-for-3d-printing
- Raspberry Pi magazine, polished finish on 3D prints: https://magazine.raspberrypi.com/articles/how-to-create-a-polished-finish-on-3d-prints
- MatterHackers, ten things to elevate your 3D prints: https://www.matterhackers.com/articles/ten-things-to-elevate-your-3d-prints
