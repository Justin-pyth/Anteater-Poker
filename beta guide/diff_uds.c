/*
 * ============================================================================
 *  MODULE DIFF:  uds.h   —  Core shared data structures
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.  Read this FIRST: every struct
 *  change here drives the function-level changes in the other diff_*.c files.
 *
 *  uds.h is the project's shared header (Card, Player, GameState, etc.).
 *  It is included by almost every .c file.  No functions live here.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- uds.h
 * ============================================================================
 */


/* ======================================================================
   PART 1 — MODIFIED (struct field additions)
   ====================================================================== */

/*
 * CHANGE 1 — Player.place  (new uint8_t field)
 *
 * ALPHA:
 *   uint8_t status;
 *   uint8_t has_cards;
 *   // place did not exist
 *
 * BETA:
 *   uint8_t status;
 *   uint8_t has_cards;
 *   uint8_t place;   // tournament finish rank: 0=still in, 1=winner, higher=busted earlier
 *
 * WHY: The new scoreboard feature (show_scoreboard in gui.c -> diff_gui.c) needs
 * to know the order in which players busted out.  place=0 means the player has
 * not finished yet; 1 means they won; 2 means second-to-last, etc.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Open uds.h and locate the Player struct.
 *   Step 2.  After the `uint8_t has_cards;` line, add:
 *              uint8_t place;
 *   Step 3.  Because place is now serialised over the network, add read/write
 *            calls in com.c — see diff_com.c.
 *   Step 4.  In resetGame() (game.c -> diff_game.c) clear place to 0 for every
 *            player so the scoreboard starts fresh on a new tournament.
 */

/*
 * CHANGE 2 — GameState: smallBlindIndex, bigBlindIndex, smallBlind, bigBlind,
 *             showdown, lastActor  (new fields)
 *
 * ALPHA GameState had (excerpt):
 *   uint8_t  dealerIndex;
 *   uint8_t  yourPlayerID;
 *   uint32_t pot;
 *   uint32_t currentBet;
 *   uint32_t minRaise;
 *   uint8_t  gameOver;
 *   uint8_t  winnerID;
 *   bool     handPlaying;
 *   bool     acted[MAX_PLAYERS];
 *
 * BETA adds between those fields:
 *   uint8_t  smallBlindIndex;  // seat that posted the small blind (MAX_PLAYERS = none)
 *   uint8_t  bigBlindIndex;    // seat that posted the big blind
 *   uint32_t smallBlind;       // configurable; defaults to SMALL_BLIND constant
 *   uint32_t bigBlind;         // configurable; defaults to BIG_BLIND constant
 *   uint8_t  showdown;         // 1 = cards revealed, waiting for .ready
 *   uint8_t  lastActor;        // seat that just moved (server-only, not encoded)
 *
 * WHY:
 *   - smallBlindIndex / bigBlindIndex: GUI tags seats "[SB]" / "[BB]".
 *   - smallBlind / bigBlind: lets the server operator change blind levels live.
 *   - showdown: lets the client display "SHOWDOWN" and reveal cards while waiting
 *     for players to press .ready instead of immediately resetting.
 *   - lastActor: defers all-in card reveal by one broadcast cycle so it looks
 *     better on screen (all-in announcement arrives before the card reveal).
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  In uds.h, add the six fields in the positions shown above.
 *            Order matters: com.c encodes them in declaration order.
 *   Step 2.  In init_server() (protocol.c -> diff_protocol.c) set:
 *              state->game.smallBlind = SMALL_BLIND;
 *              state->game.bigBlind   = BIG_BLIND;
 *   Step 3.  In resetHand() (game.c -> diff_game.c) set:
 *              gs->smallBlindIndex = MAX_PLAYERS;  // "none" sentinel
 *              gs->bigBlindIndex   = MAX_PLAYERS;
 *              gs->showdown = 0;
 *   Step 4.  In newHand() (game.c -> diff_game.c) set:
 *              gs->lastActor = MAX_PLAYERS;
 *   Step 5.  Update com.c to serialise the new fields — see diff_com.c.
 *
 * NOTE: lastActor is NOT encoded on the wire (server-only).  smallBlindIndex,
 * bigBlindIndex, smallBlind, bigBlind, and showdown ARE encoded.
 */
