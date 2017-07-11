#!/usr/bin/env swipl

/*
This is example code while allows you to connect to MQTT, subscribe to various topics, then wait for messages to
come in and react to them.  This code runs as PrologScript under SWI 7.5.2 and will handle Ctrl-C and unsubscribe/disconnect cleanly.  If you're running interactively you need to make sure you unsubscribe and disconnect before exiting or else
the code will coredump.  The wait predicate called when run as PrologScript allows messages to be processed 
indefinitely without exiting (until you send a Ctl-C).
Just add the appropriate topic subscriptions and message handlers with behavior rules and this code is
the basic outline for a home automation system central controller.
*/

:- use_module(library(mqtt)).

:- dynamic 
   mqtt_connection/1,
   subscription/1.

:- public
    quit/1,
    reload/1.

:- initialization(main,main).

main :-
    setup_signals,
    start_servers(mqtt_clientid),
    wait.

setup_signals :-
    on_signal(int,  _, quit),
    on_signal(term, _, quit),
    on_signal(hup, _, reload).

start_servers(ClientId) :-
   connect(ClientId),
   subscribe_all,
   at_halt(format('bye~n')),
   format('mqtt started~n').  

quit(_) :-
    format('Quitting on signal ~n'),
    thread_send_message(main, quit).

shutdown_servers :-
   format('mqtt shutting down~n'),  
   unsubscribe_all,
   disconnect.

reload(_) :-
    format('Reload on signal ~n'),
    thread_send_message(main, reload).

wait :-
    thread_self(Me),
    repeat,
        thread_get_message(Me, Msg),
        catch(ignore(handle_message(Msg)), E, print_message(error, E)),
        Msg == quit,
        !,
        shutdown_servers,
        halt(0).

handle_message(reload) :-
    format('handle_message reload ~n').

handle_message(quit) :-
    format('handle_message quit ~n').

/* You will need to modify this with the IP and port of the broker you will be using */
connect(ClientId) :-
   mqtt_connect(A, 'yourmqtt.broker.com', 1883, [
      alias(swi-prolog), client_id(ClientId), keepalive(120), is_async(true), 
      debug_hooks(false), hooks(true), module(user), 
      on_message(my_on_msg),
      on_disconnect(my_on_dis)
   ]),
   assert( mqtt_connection(A) ).

subscribe_all :-
   subscribe(['example','notify', '+']).

subscribe(TopicPath) :-
   is_list(TopicPath),!,
   concat_atom(TopicPath, '/', Topic),
   subscribe(Topic).

subscribe(Topic) :-
   mqtt_connection(A),
   mqtt_sub(A, Topic, []),
   assert( subscription(Topic) ).

unsubscribe_all :-
   subscription(Topic),
   unsubscribe(Topic),
   retract(subscription(Topic)),
   fail.

unsubscribe_all.

unsubscribe(Topic) :-
   mqtt_connection(Conn),
   mqtt_unsub(Conn, Topic).

publish(TopicPath) :-
   is_list(TopicPath),!,
   publish(TopicPath, '').

publish(Topic) :-
   publish(Topic, '').

publish(TopicPath, Payload) :-
   is_list(TopicPath),!,
   concat_atom(TopicPath, '/', Topic),
   mqtt_connection(A),
   mqtt_pub(A, Topic, Payload).

publish(Topic, Payload) :-
   mqtt_connection(A),
   mqtt_pub(A, Topic, Payload).

disconnect :-
   mqtt_connection(Conn),
   mqtt_disconnect(Conn),
   mqtt:c_destroy_engine.

my_on_msg(C,D) :- format('% msg > ~w - ~w~n', [C,D]).

message(['example','notify', 'outside'],_):-
   format('saw outside motion~n'),
   publish(['example', 'log'], 'saw outside motion').

message(Topic, Payload):-
   format('% msg > ~w - ~w~n', [Topic,Payload]).

message(_,_).

my_on_dis(_,_) :- format('MQTT disconnected~n').
