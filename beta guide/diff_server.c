/*
 * ============================================================================
 *  MODULE DIFF:  server.c   —  Headless server main loop
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_protocol.c (the new timed-event functions called
 *  from this loop: step_runout, tick_ready_countdown, reconcile_disconnects).
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- server.c
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: main()  (server.c)
 *
 * WHAT IT DOES: The headless server's event loop — select() on the listening
 * socket and client sockets, then dispatch.
 *
 * KEY CHANGES in the select() timeout logic:
 *
 *   ALPHA sets a 1-second timeout only when it is a bot's turn.
 *   BETA sets a 1-second timeout for THREE cases:
 *     - state.runout_pending          (all-in runout: deal a card every second)
 *     - state.ready_countdown > 0      (count down to auto-start)
 *     - bot's turn                     (now also guarded by currentPlayer < MAX_PLAYERS)
 *
 *   ALPHA on timeout (activity == 0):
 *     doOneBotTurn(); handle_after_move();
 *
 *   BETA on timeout (activity == 0):
 *     if runout_pending:             step_runout()
 *     else if ready_countdown > 0:   tick_ready_countdown()
 *     else if bot turn:              doOneBotTurn(); handle_after_move()
 *
 *   BETA also adds reconcile_disconnects() after the client I/O loop to catch
 *   connection drops signalled by send_to_client setting connected=0.
 *
 *   The dead commented-out legacy test code at the bottom of main() was removed.
 *
 * WHY:
 *   - The three-case timeout lets the server do timed work (tick countdown, deal
 *     runout cards) without blocking client I/O for more than 1 second.
 *   - reconcile_disconnects() ensures a silently-dropped client triggers the
 *     game-aware cleanup path rather than lingering as a phantom player.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Change the timeout_ptr logic to a three-way if/else-if/else-if chain.
 *   Step 2.  Change the activity == 0 block to a three-way if/else-if/else-if chain.
 *   Step 3.  Add reconcile_disconnects(&state); after the client-socket for-loop.
 *   Step 4.  Remove the commented-out legacy code block at the bottom of main().
 *
 * SEE ALSO: server_gui_main.c (diff_server_gui.c) is this loop's GUI sibling —
 * structurally identical but uses wall-clock timestamps and a 100ms GTK pump.
 */

/* (server.c has no NEW functions — all new server-side functions live in
   protocol.c; see diff_protocol.c PART 2.) */
