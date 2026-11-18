# Automatic label when an element is placed

When you place an element on a folio, QElectroTech gives it a label of the
form `<prefix><number>`, for example `K1`, `K2`, `F1`, `A1`.

## Rules

- The number is the **smallest free number** for that prefix in the whole
  project. If `K1` and `K3` are taken, the next contactor gets `K2`.
- A label you typed, or one stored in the element definition, is **never
  overwritten**. An empty label and the placeholder `_` count as "no label".
- Slave elements (cross references) and reports are **not numbered**: they
  take the label of their master.
- If the project already has an element numbering scheme with a formula
  (Project properties, Autonumbering), that scheme is used and this feature
  does nothing. Existing projects behave as before.
- One Ctrl+Z removes the element together with its label.
- Switch it off in Settings, General: "Label an element automatically when
  placing it". It is on by default.

## Where the prefix comes from

1. The prefix of the element's collection folder (`qet_labels.xml`, ISO 81346
   letters such as K, Q, F, S, H, M, T, G, X). An empty prefix there means
   "this family has no label": the element is left unlabelled.
2. Otherwise, a fallback by element kind:

   | Element                                  | Prefix |
   | ---------------------------------------- | ------ |
   | Folder `15_beckhoff_ethercat` (any card) | A      |
   | Terminal                                 | X      |
   | Master, coil                             | K      |
   | Master, protection                       | Q      |
   | Master, commutator                       | S      |
   | Master, PLC                              | A      |

   Other simple elements get no label.

The code is in `sources/autoNum/autolabel.cpp` (rules) and
`Element::giveAutoLabel()` (hook, called from `Element::setUpFormula()` when
the element is placed).

## Limits

- Copy/paste and duplication keep the labels of the source elements.
- Labels have no leading dash (`K1`, not `-K1`), like the prefixes of
  `qet_labels.xml` and the island labels.
- The match is on the exact text: `A1.10` does not make `A1` taken.
- Beckhoff cards get `A1`, `A2`, ... The "Number the cards of an island"
  command still renumbers them as `A1.10`, `A1.20`.
- Protection masters get `Q`; use `qet_labels.xml` to get `F` for a family.
