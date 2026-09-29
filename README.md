# Wii Homebrew Games

אוסף משחקי הומברו (homebrew) מקוריים ל-Wii, שרצים גם על קונסולת Wii אמיתית (עם ה-Homebrew
Channel) וגם באמולטור [Dolphin](https://dolphin-emu.org/). המשחקים נכתבים ב-C עם
[devkitPro](https://devkitpro.org/) / devkitPPC / libogc, ומדגימים שימוש ב-Wii Remote (כולל
מצביע ה-IR ותאוצה) וב-Wii Balance Board.

> **הערה חשובה על זכויות יוצרים:** הריפו הזה מכיל אך ורק משחקים מקוריים שנכתבו כאן. אין כאן, ולא
> יהיו כאן, קבצי ROM/ISO של משחקים מסחריים (כמו Mario), ולא הוראות להשגה או הרצה של עותקים פרוצים
> של משחקים כאלה — זו הפרת זכויות יוצרים ואני לא עוזר עם זה. מה שכן יש כאן זה תהליך ה-Homebrew
> Channel הרשמי והחוקי, שמאפשר להריץ תוכנה עצמאית (כמו המשחקים בריפו הזה) על ה-Wii שלך.

## מבנה הריפו

```
games/
  01-pointer-shooter/   משחק מצביע IR - כוונון ויריה למטרות
  02-balance-quest/     משחק Balance Board - הטיית משקל לפי כיוון מבוקש
  03-swing-slash/       משחק תאוצה - נפנוף ה-Wii Remote כדי "לחתוך" אויבים
  04-hop-run/           משחק פלטפורמר מקורי (ריצה אינסופית וקפיצות מעל מכשולים)
  05-quiz-buzzer/       משחק מסיבה - עד 4 שלטים, מי לוחץ A ראשון
  06-duck-hunt/         משחק יריה מקורי - מטרות נעות + מצביע IR
  07-rhythm-tap/        משחק קצב - לחיצת A בדיוק בזמן הנכון
  08-simon-says/        משחק זיכרון (Simon) עם 4 כפתורים
  09-island-adventure/  משחק מורכב, 3 עולמות/שלבים - Balance Board + שלט יחד
  10-connect-four/      4 בשורה צבעוני לשני שחקנים על אותו שלט
  template/             שלד ריק להתחלת משחק חדש
.github/workflows/      בנייה אוטומטית ב-CI (Docker של devkitPro) לכל משחק
```

## התקנת סביבת הפיתוח (Windows)

1. הורידו והריצו את מתקין devkitPro:
   https://github.com/devkitPro/installer/releases/latest (`devkitProUpdater-X.Y.Z.exe`).
   ההתקנה דורשת אישור הרשאות מנהל (UAC) - יש להריץ אותה ידנית ולעבור את האשף (ברירת המחדל
   `C:\devkitPro` מומלצת).
2. בתפריט Start יופיע קיצור **devkitPro Pacman** (או `msys2.exe` בתוך `C:\devkitPro\msys2`).
   פתחו אותו והריצו:
   ```
   pacman -Syu
   pacman -S wii-dev
   ```
   זה מתקין את devkitPPC, libogc, וכל ספריות ה-Wii הדרושות (כולל WPAD, שדרכה קוראים את
   ה-Wii Remote וה-Balance Board).
3. ודאו שמשתני הסביבה מוגדרים (המתקין בד"כ עושה את זה אוטומטית):
   ```
   DEVKITPRO=C:\devkitPro
   DEVKITPPC=C:\devkitPro\devkitPPC
   ```

## בנייה

מתוך שורת פקודה עם המשתנים הנ"ל (למשל ה-MSYS2 shell של devkitPro):

```sh
cd games/01-pointer-shooter
make
```

זה מייצר קובץ `01-pointer-shooter.dol` - זה קובץ ההרצה של המשחק.

כל משחק בריפו נבנה גם אוטומטית ב-GitHub Actions (רואים את זה בטאב Actions של הריפו), כך
שאפשר לוודא שהקוד תקין גם בלי סביבת פיתוח מקומית.

## הרצה

### על Wii אמיתי (עם Homebrew Channel)

מכיוון שכבר יש לכם Homebrew Channel מותקן על ה-Wii:

1. חברו כרטיס SD למחשב.
2. צרו בכרטיס SD תיקייה `apps\<שם-המשחק>\` (למשל `apps\01-pointer-shooter\`).
3. העתיקו לתוכה את הקובץ שנוצר בבנייה ושנו את שמו ל-`boot.dol`.
4. (אופציונלי אך מומלץ) הוסיפו קובץ `meta.xml` קטן באותה תיקייה עם שם/תיאור המשחק, כדי שהוא
   יופיע יפה ברשימת ה-Homebrew Channel:
   ```xml
   <app version="1">
     <name>Pointer Shooter</name>
     <coder>ayal408</coder>
     <version>1.0</version>
     <short_description>Aim with the Wiimote and shoot targets</short_description>
   </app>
   ```
5. הכניסו את הכרטיס ל-Wii, פתחו את ה-Homebrew Channel, ובחרו את המשחק.

### באמולטור Dolphin

פותחים את Dolphin ← File ← Open ← בוחרים את קובץ ה-`.dol`. אפשר גם לגרור את הקובץ ישירות
לחלון Dolphin. כדי לדמות Wii Remote/Balance Board דרך המחשב, מגדירים ב-Dolphin תחת
`Controllers` את סוג הקלט (Emulated Wiimote עם מקלדת/עכבר/ג'ויסטיק, או Real Wiimote דרך
Bluetooth אם יש למחשב Bluetooth).

## חיבור וסנכרון Wii Remote ו-Balance Board

* **סנכרון (pairing):** פותחים את המכסה בתחתית ה-Wii Remote/Balance Board ולוחצים על כפתור
  ה-SYNC האדום. תוך כמה שניות לוחצים גם על כפתור ה-SYNC בקונסולת ה-Wii (מתחת למכסה ה-SD, או
  ליד יציאות ה-USB בדגמים ישנים). הפעולה צריכה רק להתבצע פעם אחת - הבקר "זוכר" את הקונסולה.
* **Balance Board:** מזוהה בקוד בדיוק כמו Wii Remote (יש לו גם כפתור אדום פנימי לחיבור/כיבוי),
  אבל מזוהה כ"הרחבה" (`expansion`) מסוג `WPAD_EXP_BALANCE_BOARD` עם 4 חיישני משקל (קדמי-שמאל,
  קדמי-ימין, אחורי-שמאל, אחורי-ימין).
* לקוד לא אכפת אם ה-Balance Board מחובר בערוץ (channel) 0 או אחר - בדוגמאות כאן נעשה שימוש
  ב-`WPAD_CHAN_0` לפשטות; במשחק עם כמה שחקנים אפשר לקרוא גם מ-`WPAD_CHAN_1`..`WPAD_CHAN_3`.

## תרגום מהיר של ה-API (WPAD, מתוך libogc)

| מה רוצים לקרוא | קוד |
|---|---|
| אתחול | `WPAD_Init();` |
| קריאת מצב כל הבקרים (בכל לולאת פריים) | `WPAD_ScanPads();` |
| כפתורים שנלחצו הרגע | `u32 p = WPAD_ButtonsDown(WPAD_CHAN_0);` |
| כפתורים מוחזקים | `u32 h = WPAD_ButtonsHeld(WPAD_CHAN_0);` |
| מצביע IR (איפה מכוון ה-Wiimote על המסך) | `struct ir_t ir; WPAD_IR(WPAD_CHAN_0, &ir);` (`ir.valid`, `ir.x`, `ir.y`) |
| תאוצה (נפנוף/הטיה) | `WPADData *d = WPAD_Data(WPAD_CHAN_0); d->accel.x/y/z` |
| הרחבה מחוברת (Nunchuk / Balance Board / Classic Controller) | `struct expansion_t e; WPAD_Expansion(WPAD_CHAN_0, &e); e.type` |
| משקל מ-Balance Board (ק"ג, לכל חיישן) | `e.wb.tl`, `e.wb.tr`, `e.wb.bl`, `e.wb.br` (אחרי `e.type == WPAD_EXP_BALANCE_BOARD`) |

דוגמאות מלאות ועובדות לכל אחד מהמקרים האלה נמצאות בקבצי ה-`source/main.c` של המשחקים.

## משחק מורכב עם שלבים ועולמות: 09-island-adventure

זה המשחק הכי שלם בריפו, ומדגים שילוב של Balance Board ו-Wii Remote **יחד** באותו משחק,
פרוסים ל-3 "עולמות" עם התקדמות בין שלבים:

* **הטיית משקל על ה-Balance Board** (ימינה/שמאלה) מזיזה אתכם בין 3 מסלולים ("lanes") כדי
  להתחמק ממכשולים.
* **כפתור A בשלט** גורם לקפיצה מעל מכשולים נמוכים.
* אחרי שעוברים כמות מסוימת של מכשולים בעולם נתון, נפתח "שער" - צריך לעמוד **ממורכזים**
  (משקל שווה) על ה-Balance Board במשך כ-1.5 שניות כדי לפתוח אותו ולעבור לעולם הבא.
* 3 עולמות (Meadow Path, Desert Dunes, Volcano Ridge) - כל אחד מהיר וצפוף יותר מהקודם.
* יש חיים (lives), ניקוד, ומסך "ניצחתם את ההרפתקה" בסיום העולם השלישי.

זו גם דוגמה טובה למבנה state machine (PLAYING / GATE / WORLD_CLEAR / GAMEOVER / WIN) שאפשר
להעתיק ולהרחיב למשחקים מורכבים נוספים עם עוד עולמות ושלבים.

## איך יוצרים משחק נוסף

1. העתיקו את כל תיקיית `games/template` לתיקייה חדשה, למשל `games/04-my-new-game`.
2. פתחו את `source/main.c` ושנו את הלוגיקה (המבנה הבסיסי - אתחול וידאו, אתחול WPAD, לולאת
   `while(1)` שקוראת קלט ומציירת/מדפיסה - כבר מוכן).
3. אם המשחק צריך Balance Board, העתיקו את קטע `WPAD_Expansion` מ-`02-balance-quest`.
   אם הוא צריך תאוצה/נפנוף, העתיקו מ-`03-swing-slash`. אם הוא צריך מצביע IR, מ-`01-pointer-shooter`.
4. בנו עם `make` בתוך התיקייה החדשה, בדיוק כמו בשאר המשחקים.
5. הוסיפו את הנתיב החדש (`games/04-my-new-game`) למטריצת ה-build ב-
   `.github/workflows/build.yml` כדי שגם הוא ייבנה אוטומטית ב-CI.

### רעיונות למשחקים נוספים (מנוע קלט שכבר קיים בריפו)

* **Balance Board:** משחק גלישה/סקי (הטיה = פנייה), משחק יוגה/איזון עם רצף תנוחות, משחק
  "רצפת לבה" (קפיצות/העברת משקל מהירה כדי לא "ליפול"), מד כושר עם ניקוד יומי.
* **מצביע IR:** משחק ציד ברווזים, "צבע את המסך", תפריט/ציור עם לייזר-פוינטר וירטואלי.
* **תאוצה (נפנוף):** התעמלות עם ספירת חזרות, תופים (נפנוף בקצב), "כישוף" - ציור תנועה באוויר.
* **מולטיפלייר:** עד 4 Wii Remotes (`WPAD_CHAN_0`-`WPAD_CHAN_3`) למשחקי מסיבה תחרותיים.

## CI

כל push/PR מריץ בנייה של כל המשחקים בתוך container רשמי של devkitPro
(`devkitpro/devkitppc`), ומעלה את קבצי ה-`.dol` כ-artifacts - כך אפשר לבדוק שהקוד קומפילירי
תקין גם בלי סביבת devkitPro מותקנת מקומית.
