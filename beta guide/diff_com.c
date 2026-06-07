/*
 * ============================================================================
 *  MODULE DIFF:  com.c / com.h   —  Wire encoding / decoding
 *  ALPHA (gladeUI @200932f)  ->  BETA (test @aa84c01)
 * ============================================================================
 *
 *  TEACHING ARTIFACT — not compilable code.
 *  Prerequisite reading: diff_uds.c (the new Player/GameState fields these
 *  functions serialise).
 *
 *  CONTEXT: com.c implements a simple binary framing protocol.
 *    encode_player_data()  serialises one Player struct into bytes;
 *    decode_player_data()  does the reverse.
 *    encode_server_data() / decode_server_data() do the same for GameState.
 *    prepare_payload() / receive_payload() handle complete Message objects.
 *
 *  IMPORTANT: encode and decode MUST write/read fields in identical order.
 *  Add a field to one -> add it to the other at the SAME position, or the
 *  client decodes garbage.
 *
 *  Reproduce the raw diff from inside /home/duc/Anteater-Poker/beta:
 *      git diff 200932f aa84c01 -- com.c com.h
 * ============================================================================
 */


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * $   PART 1 — MODIFIED FUNCTIONS                                          $
 * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/*
 * MODIFIED FUNCTION: encode_player_data() / decode_player_data()
 *
 * CHANGE: A new Player.place byte is serialised between has_cards and chips.
 *   ALPHA encode order: ... status, has_cards, chips, current_bet
 *   BETA  encode order: ... status, has_cards, place, chips, current_bet
 *
 * WHY: The client needs place to show the scoreboard rankings.
 *
 * HOW TO IMPLEMENT:
 *   In encode_player_data(), after  write_u8(buffer, offset, player->has_cards);
 *   add:  write_u8(buffer, offset, player->place);
 *
 *   In decode_player_data(), after  read_u8(buffer, offset, &player->has_cards);
 *   add:  read_u8(buffer, offset, &player->place);
 *
 *   Both functions are mirrored; that is the entire change.
 */

/*
 * MODIFIED FUNCTION: encode_server_data() / decode_server_data()
 *
 * CHANGE: Five new GameState fields are serialised.
 *
 *   ALPHA encode order (excerpt):
 *     ... stage, currentPlayer, dealerIndex, yourPlayerID,
 *         pot, currentBet, minRaise,
 *         handPlaying, gameOver, winnerID
 *
 *   BETA encode order (excerpt):
 *     ... stage, currentPlayer, dealerIndex,
 *         smallBlindIndex, bigBlindIndex,        << NEW
 *         yourPlayerID,
 *         pot, currentBet, minRaise,
 *         smallBlind, bigBlind,                  << NEW
 *         handPlaying, gameOver, winnerID,
 *         showdown                               << NEW
 *
 * WHY: All five fields were added to GameState (uds.h); the wire encoding must
 * transmit them so the GUI can use them.  (lastActor is server-only and is NOT
 * encoded.)
 *
 * HOW TO IMPLEMENT:
 *   In encode_server_data(), insert in the positions shown:
 *     write_u8(buffer, offset, gameState->smallBlindIndex);
 *     write_u8(buffer, offset, gameState->bigBlindIndex);   // after dealerIndex
 *     write_u32(buffer, offset, gameState->smallBlind);
 *     write_u32(buffer, offset, gameState->bigBlind);       // after minRaise
 *     write_u8(buffer, offset, gameState->showdown);        // after winnerID
 *
 *   Mirror every write_u8 / write_u32 with read_u8 / read_u32 in
 *   decode_server_data() at the SAME position.
 *
 * PITFALL: If you add to encode but forget decode (or vice versa), the decoded
 * GameState is shifted by one field for everything that follows — completely
 * wrong game state on the client.  Always update both together and run
 * `make test` to catch mismatches.
 */

/*
 * MODIFIED FUNCTION: prepare_payload() / receive_payload()  — MSG_TYPE_JOIN
 *
 * CHANGE: A new message type MSG_TYPE_JOIN = 8 is added to com.h's MessageType
 * enum.  The encode/decode switch statements handle it using the existing
 * encode_chat_message / decode_chat_message helpers (same wire format as chat).
 *
 *   ALPHA: enum had 8 values (0-7), default case returned -1.
 *   BETA:  MSG_TYPE_JOIN = 8 added; prepare_payload / receive_payload cases added.
 *
 * WHY: When a client first connects it sends its display name so the server can
 * announce "Alice has joined!".  Reusing the chat wire format (sender_id + text)
 * avoids duplicating serialisation code.
 *
 * HOW TO IMPLEMENT:
 *   Step 1.  In com.h, add MSG_TYPE_JOIN = 8 to the MessageType enum (add a
 *            trailing comma after MSG_CD_SIGNAL = 7 first).
 *   Step 2.  In prepare_payload(), add a case for MSG_TYPE_JOIN -> encode_chat_message().
 *   Step 3.  In receive_payload(), add a case for MSG_TYPE_JOIN -> decode_chat_message().
 */
