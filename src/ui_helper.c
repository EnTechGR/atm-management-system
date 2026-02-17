#include "header.h"
#include "ui/tui.h"

/* ── success() ───────────────────────────────────────────────────────────────
   Shows a "what next?" modal after every successful operation.
   Returns immediately — the mainMenu() while-loop handles redrawing.
   This avoids the previous stack-building recursion
   (success → mainMenu → op → success → mainMenu → …).
   ─────────────────────────────────────────────────────────────────────────── */
void success(struct User u) {
    (void)u;   /* u is kept in the signature for API compatibility */

    const char *opts[] = {
        "Return to Main Menu",
        "Exit Application",
        NULL
    };
    int choice = tui_modal_menu("DONE", opts, "ENTER to confirm");
    if (choice == 1) {
        tui_cleanup();
        exit(0);
    }
    /* choice == 0 or ESC: just return.
       mainMenu's while(1) loop will redraw the menu on the next iteration. */
}

/* ── stayOrReturn() ──────────────────────────────────────────────────────────
   Called when a record is not found or a sub-operation fails.
   notGood == 0  → show "Record not found" + three choices.
   notGood != 0  → show two choices.
   ─────────────────────────────────────────────────────────────────────────── */
void stayOrReturn(int notGood, void (*f)(struct User), struct User u) {
    if (notGood == 0) {
        tui_modal_error("Record not found!");

        const char *opts[] = {
            "Try Again",
            "Return to Main Menu",
            "Exit Application",
            NULL
        };
        int choice = tui_modal_menu("WHAT NEXT?", opts, "ENTER to confirm");
        switch (choice) {
            case 0:  if (f) { f(u); } return;
            case 1:  return;   /* mainMenu loop handles the rest */
            default: tui_cleanup(); exit(0);
        }
    } else {
        const char *opts[] = {
            "Return to Main Menu",
            "Exit Application",
            NULL
        };
        int choice = tui_modal_menu("WHAT NEXT?", opts, "ENTER to confirm");
        if (choice == 1) { tui_cleanup(); exit(0); }
        /* else: return, mainMenu loop continues */
    }
}