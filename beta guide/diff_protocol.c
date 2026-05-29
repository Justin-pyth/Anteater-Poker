/*
 * ============================================================================
 *  MODULE DIFF:  protocol.c / protocol.h   —  Server logic & I/O
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_uds.c, diff_game.c, diff_com.c.
 *  This is the heart of the beta feature work (blinds, showdown flow, .ready
 *  countdown, all-in runout, mid-match join/leave).
 *
 *  protocol.h gains THREE new ServerState fields used below:
 *      int  runout_pending;     // 1 while dealing an all-in board, one card/sec
 *      int  ready_countdown;    // seconds remaining before auto-start (0 = off)
 *      void (*chat_notify)(const char *sender, const char *msg);  // GUI hook, NULL on headless
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- protocol.c protocol.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: handle_client_communication()
 *
 * WHAT IT DOES: Reads one message from a client's socket and dispatches it
 * (MOVE, CHAT, READY, and now JOIN).
 *
 * CHANGES:
 *   A. playerID override — before calling tryMove():
 *      ALPHA: used data.action.playerID from the decoded payload (client-supplied).
 *      BETA:  data.action.playerID = client->id; (server assigns from the slot)
 *      WHY: prevents a client submitting a move as another player; client->id
 *      is authoritative.
 *
 *   B. READY handler — spectating support:
 *      ALPHA: always set PLAYER_READY unconditionally.
 *      BETA:  chips > 0 -> PLAYER_READY; else PLAYER_SPECTATING.
 *      WHY: a busted-out player should watch, not block the table.
 *
 *   C. READY handler — announce readiness:
 *      BETA adds a broadcast_chat_message() saying "Alice is ready!" or
 *      "Alice is spectating this round." depending on resulting status.
 *
 *   D. READY handler — spectators excluded from "all-ready" count:
 *      ALPHA: readyClients == connectedClients triggers start.
 *      BETA:  spectatingClients tracked separately; condition is
 *             (readyClients == connectedClients) && (readyClients + spectatingClients >= 1)
 *
 *   E. READY handler — ready_countdown cancellation:
 *      BETA sets state->ready_countdown = 0 when all players ready up early.
 *
 *   F. New MSG_TYPE_JOIN handler:
 *      Copies the player name from data.chat into both game.players[id].name and
 *      clients[id].name, broadcasts a "has joined!" announcement and the state.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Add the playerID override as the first line in the MSG_TYPE_ACTION
 *            branch, before tryMove().
 *   Step 2.  In the MSG_TYPE_READY branch, add the chips>0 check (READY vs SPECTATING).
 *   Step 3.  Add the announcement broadcast after the status assignment.
 *   Step 4.  Split the connected-client counting loop to also count spectators.
 *   Step 5.  Adjust the "all ready?" condition as shown.
 *   Step 6.  Add state->ready_countdown = 0; in the all-ready branch.
 *   Step 7.  Call start_new_hand(state) instead of inlining the reset logic.
 *   Step 8.  Add the MSG_TYPE_JOIN else-if block after MSG_TYPE_READY.
 */

/*
 * MODIFIED FUNCTION: init_server()
 *
 * WHAT IT DOES: Zero-initialises the ServerState struct before first use.
 *
 * CHANGE: New fields initialised:
 *   state->runout_pending  = 0;
 *   state->ready_countdown = 0;
 *   state->chat_notify     = NULL;
 *   state->game.smallBlind = SMALL_BLIND;
 *   state->game.bigBlind   = BIG_BLIND;
 *
 * WHY:
 *   - runout_pending / ready_countdown: drive timed events in the main loop.
 *   - chat_notify: NULL means the headless server won't call a missing GUI hook.
 *   - smallBlind / bigBlind: initialise blind levels to compile-time defaults;
 *     server_gui can change them later.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Add the three runout/countdown/chat lines inside init_server().
 *   Step 2.  After initDeck(), set the two blind fields.
 *   Step 3.  In protocol.h, add the three new ServerState fields (see file header).
 */

/*
 * MODIFIED FUNCTION: add_connection()
 *
 * WHAT IT DOES: Assigns a free slot to a newly accepted TCP client and
 * broadcasts a welcome message.
 *
 * CHANGE: The welcome message was split into two parts:
 *   ALPHA: "Welcome to Anteater Poker! Type /ready to ready up."  (broadcast)
 *   BETA:
 *     1. "A new player has connected! (N player(s) in lobby)"   (broadcast)
 *     2. "Welcome to Anteater Poker! Type .ready to ready up."  (hello, after)
 *
 * WHY: a public lobby-count announcement plus a private instructions message
 * improves UX.  The command also changed from /ready to .ready.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Build the join_msg string with the player count and broadcast it.
 *   Step 2.  Then broadcast the hello with the updated .ready text.
 */

/*
 * MODIFIED FUNCTION: hide_card_info_for_others()
 *
 * WHAT IT DOES: Before broadcasting a GameState to a specific client, blanks out
 * every other player's hole cards so they cannot cheat.
 *
 * ALPHA:
 *   if (!game->handPlaying) return;   // don't hide if no hand is active
 *   for each player != player_id: blank their hand
 *
 * BETA:
 *   (no early return — caller checks showdown)
 *   for each player != player_id:
 *       if player is ALL_IN AND index != lastActor: continue;  // reveal all-in
 *       blank their hand
 *
 * WHY:
 *   - The !handPlaying early-return was removed; broadcast_game_state() now only
 *     calls this when showdown==0 and the viewer is not a spectator, so the guard
 *     is redundant.
 *   - All-in players' cards are revealed to the table — but not on the same
 *     broadcast they went all-in (lastActor).  The `i != lastActor` check defers
 *     the reveal by one cycle so the animation looks correct.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Remove the `if (!game->handPlaying) return;` guard.
 *   Step 2.  Convert the loop to a continue-based filter:
 *              if (i == player_id) continue;
 *              if (game->players[i].status == PLAYER_ALL_IN && i != game->lastActor) continue;
 *              // blank hand[0] and hand[1]
 *   Step 3.  Signature unchanged.
 */

/*
 * MODIFIED FUNCTION: broadcast_game_state()
 *
 * WHAT IT DOES: Sends a copy of the current GameState to every connected client,
 * personalised with their player ID and opponents' cards blanked.
 *
 * CHANGES:
 *   1. Spectators see all cards (skip the hide step for spectating clients).
 *   2. Showdown state: if showdown==1, all cards revealed (skip hide step).
 *   3. Folded and spectating players' cards are always hidden:
 *        for (j in all players):
 *            if status == FOLDED or SPECTATING: blank hand + has_cards = 0
 *
 * WHY:
 *   - Spectators see all hole cards (design choice).
 *   - During showdown, everyone sees everyone's cards.
 *   - Folded players forfeited the right to reveal; SPECTATING players aren't in
 *     the hand, so neither should show cards even at showdown.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Before hide_card_info_for_others(), compute:
 *              int client_is_spectator =
 *                (state->game.players[state->clients[i].id].status == PLAYER_SPECTATING);
 *   Step 2.  Guard the call:
 *              if (!state->game.showdown && !client_is_spectator)
 *                  hide_card_info_for_others(&temp_data.gameState, state->clients[i].id);
 *   Step 3.  After it, add the folded/spectating wipe loop.
 */

/*
 * MODIFIED FUNCTION: broadcast_chat_message()
 *
 * WHAT IT DOES: Sends a text message to all connected clients.
 *
 * CHANGE: After the client send loop, call the optional server GUI chat hook:
 *   if (state->chat_notify) {
 *       // resolve sender name (SERVER for system msgs, player name otherwise)
 *       state->chat_notify(sender_name, message);
 *   }
 *
 * WHY: the server GUI window has a chat log panel.  Rather than duplicate the
 * send loop, the server calls a function pointer the GUI registers.  Headless
 * server leaves chat_notify == NULL, making the if-block a no-op.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  After the send_to_client() loop, add the if-block shown.
 *   Step 2.  Resolve the sender name: sender_id >= MAX_PLAYERS -> "SERVER";
 *            else game.players[sender_id].name (or "Player N" if empty).
 *   Step 3.  Call state->chat_notify(resolved_name, message).
 */

/*
 * MODIFIED FUNCTION: handle_after_move()
 *
 * WHAT IT DOES: Called after any player action (move, disconnect, bot turn) to
 * decide what to broadcast / do next.
 *
 * ALPHA (paraphrased):
 *   if gameOver: broadcast and return
 *   if !handPlaying:
 *       if remainingPlayers == 1: set gameOver, broadcast, return
 *       broadcast + sleep(3) + start_new_hand()
 *       return
 *   broadcast state + countdown signal
 *
 * BETA (paraphrased):
 *   if gameOver: broadcast and return
 *   if !handPlaying:
 *       updatePlaces(gs)                  // assign finishing ranks
 *       if remainingPlayers == 1: set gameOver (no return yet)
 *       broadcast game state              // show showdown / final state
 *       if !ready_countdown:
 *           state->ready_countdown = 30   // start 30s auto-start countdown
 *           broadcast "Hand over! Type .ready..."
 *       return
 *   if all-in runout (findActive returns 0):
 *       clear lastActor (defer over)
 *       state->runout_pending = 1
 *       broadcast and return
 *   broadcast countdown + game state
 *
 * WHY:
 *   - sleep(3): REMOVED.  Blocking the main loop for 3s freezes ALL client I/O.
 *     The async ready countdown replaces it.
 *   - updatePlaces: called here (the guaranteed post-hand path).
 *   - ready_countdown: prevents the server sitting idle if a player walks away.
 *   - runout_pending: triggers timed board-card dealing in the main loop.
 *   - broadcast_cd_signal moved AFTER broadcast_game_state so clients get the
 *     state before the countdown prompt.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  At the top of the !handPlaying branch, call updatePlaces().
 *   Step 2.  Keep gameOver detection but DON'T return immediately — fall through.
 *   Step 3.  Remove sleep(3) and the start_new_hand() call from this function.
 *   Step 4.  Add the ready_countdown init block (check it is 0 first).
 *   Step 5.  Add the all-in runout detection block before the normal broadcast.
 *   Step 6.  Swap the order of broadcast_game_state and broadcast_cd_signal.
 */

/*
 * MODIFIED FUNCTION: start_new_hand()
 *
 * WHAT IT DOES: Sets up state for a fresh hand — resets slots, adds bots, calls
 * newHand().  Shared by the READY handler and the auto-start countdown.
 *
 * ALPHA (simplified):
 *   if gameOver: resetGame()
 *   for each slot: connected+chips>0 -> READY; not connected -> EMPTY
 *   addBot(); newHand(); broadcast_game_state(); broadcast_cd_signal()
 *
 * BETA (simplified):
 *   if gameOver:
 *       resetGame()
 *       re-sync all slots (connected->READY, not connected->EMPTY)
 *   else if showdown:
 *       resetHand()              // <-- explicit resetHand before the new hand
 *       re-sync connected slots with chips > 0 to READY
 *       free disconnected seats (set to EMPTY)
 *   addBot(); newHand(); broadcast_cd_signal()
 *   // NOTE: broadcast_game_state() is now called by the CALLER, not here
 *
 * WHY:
 *   - Two-path reset: full resetGame() if the tournament is over; only resetHand()
 *     if one hand ended (preserve chip counts, names, etc.).
 *   - Disconnected seat cleanup in the showdown path frees seats for new players.
 *   - broadcast_game_state() removed here: callers (tick_ready_countdown, the
 *     READY handler) broadcast after, to avoid double-broadcasting.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  Wrap the existing gameOver reset in `if (state->game.gameOver)`.
 *   Step 2.  Add an `else if (state->game.showdown)` block: resetHand() +
 *            re-sync connected/disconnected slots.
 *   Step 3.  Keep addBot() and newHand() after both branches.
 *   Step 4.  Move broadcast_game_state() and broadcast_cd_signal() to the callers.
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 2 — NEW FUNCTIONS                                               $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * NEW FUNCTION: handle_player_leave()
 *   void handle_player_leave(ServerState *state, Client *client)
 *
 * WHAT IT DOES: Game-aware disconnect handler.  When a client's connection drops
 * (during or between hands), it:
 *   1. Captures hand context (was the player in the hand? was it their turn?).
 *   2. Calls remove_client() to mark them DISCONNECTED and close the socket.
 *   3. Broadcasts a "X left the game" message.
 *   4. If they were an active contestant:
 *      - If only one contestant remains, award them the pot (mirrors processMove).
 *      - If it was their turn, call processMove() from their seat to advance.
 *   5. Calls handle_after_move() to broadcast the updated state.
 *
 * DECLARED IN: protocol.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  int id = client->id.  Guard: if out of range -> remove_client(); return.
 *   Step 2.  Read status and handPlaying BEFORE remove_client() (which marks the
 *            player DISCONNECTED and loses the previous status).
 *   Step 3.  Capture the player's name for the announcement.
 *   Step 4.  remove_client(state, client).
 *   Step 5.  broadcast_chat_message() the leave message.
 *   Step 6.  If wasContestant:
 *     a. findActive to check the 1-contestant case.
 *     b. If 1 contestant: award pot, set winnerID + showdown + handPlaying=false.
 *     c. Else if wasTheirTurn: processMove(gs, deck, id) to advance the turn.
 *   Step 7.  handle_after_move(state) to broadcast.
 *   Step 8.  If not a contestant, just broadcast_game_state() directly.
 *
 * WHY NEEDED: Without this, a player who disconnects mid-hand keeps their PLAYING
 * status, and the table waits for their turn forever (or 90s until auto-fold).
 */

/*
 * NEW FUNCTION: reconcile_disconnects()
 *   void reconcile_disconnects(ServerState *state)
 *
 * WHAT IT DOES: Called once per main-loop iteration after all client I/O.  Finds
 * any client where connected==0 but client_fd >= 0 (connection dropped silently —
 * send_to_client set connected=0 on a write error) and routes them through
 * handle_player_leave().
 *
 * DECLARED IN: protocol.h
 *
 * IMPLEMENTATION GUIDE:
 *   Loop over all MAX_CLIENTS slots.  For each:
 *   - Skip if connected (no issue).
 *   - Skip if client_fd < 0 (bot or empty slot — never had a real socket).
 *   - Skip if id out of range.
 *   - Skip if player status is already DISCONNECTED or EMPTY.
 *   - Otherwise: handle_player_leave(state, c).
 *
 * WHY THE client_fd CHECK IS CRITICAL:
 *   Bot players have PLAYING status but client_fd == -1 (no TCP connection).
 *   Without the fd check, reconcile_disconnects would treat every bot as a
 *   dropped connection and instantly end the hand.  remove_client() resets fd to
 *   -1 after processing, so processed slots are skipped on future passes.
 */

/*
 * NEW FUNCTION: step_runout()
 *   void step_runout(ServerState *state)
 *
 * WHAT IT DOES: Called once per second by the main loop when runout_pending==1.
 * Advances the board by one stage (via resolveNoActStep()), broadcasts the new
 * state, and clears runout_pending when the hand is over.
 *
 * DECLARED IN: protocol.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  resolveNoActStep(&state->game, &state->deck).  If it returns false
 *            (active players remain — shouldn't happen during a runout, but be
 *            defensive): clear runout_pending and return.
 *   Step 2.  If !state->game.handPlaying: board complete.  Clear runout_pending
 *            and call handle_after_move(state).
 *   Step 3.  Else: broadcast_game_state(state) to show the new community card.
 *
 * WHY: the main loop uses a 1-second select() timeout when runout_pending is set.
 * On each timeout, step_runout() deals one more card -> visible one-at-a-time
 * runout effect.
 */

/*
 * NEW FUNCTION: tick_ready_countdown()
 *   void tick_ready_countdown(ServerState *state)
 *
 * WHAT IT DOES: Called once per second when ready_countdown > 0.  Decrements the
 * counter, announces milestones (20, 10, 5, 3, 2, 1 seconds), and at 0 moves AFK
 * players to SPECTATING and calls start_new_hand().
 *
 * DECLARED IN: protocol.h
 *
 * IMPLEMENTATION GUIDE:
 *   Step 1.  Guard: if ready_countdown <= 0, return.
 *   Step 2.  Decrement ready_countdown.
 *   Step 3.  At milestone values, broadcast "Starting in N second(s)...".
 *   Step 4.  If ready_countdown > 0, return (still running).
 *   Step 5.  At 0: loop over connected clients.  If a player is neither READY nor
 *            SPECTATING, set them SPECTATING and broadcast "X was moved to
 *            spectator (AFK)."
 *   Step 6.  Broadcast "Starting now!".
 *   Step 7.  start_new_hand(state).
 *   Step 8.  broadcast_game_state(state).
 *
 * WHY: prevents the table freezing if a player doesn't press .ready after a hand.
 * AFK players become spectators (not kicked) so they can rejoin by pressing .ready.
 */
