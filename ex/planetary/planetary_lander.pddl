(define (domain power)
(:requirements :typing :durative-actions :fluents :time :negative-preconditions :timed-initial-literals)
(:types equipment)
(:predicates (day) (commsOpen) (readyForObs1) (readyForObs2)
(gotObs1) (gotObs2)
(available ?e - equipment))
(:functions (demand) (supply) (soc) (charge_rate) (daytime)
(heater_rate) (dusk) (dawn)
(fullprepare_durtime) (prepareobs1_durtime) (prepareobs2_durtime)
(observe1_durtime) (observe2_durtime) (obs1_rate) (obs2_rate)
(A_rate) (B_rate) (C_rate) (D_rate) (safeLevel)
(solar_const))
(:process charging
:parameters ()
:precondition (and (< (demand) (supply)) (day))
:effect (and (increase (soc) (* #t (* (* (- (supply) (demand))
(charge_rate))
(- 100 (soc)))
)))
)
(:process discharging
:parameters ()
:precondition (> (demand) (supply))
:effect (decrease soc (* #t (- (demand) (supply))))
)
(:process generating
:parameters ()
:precondition (day)
:effect (and (increase (supply)
(* #t (* (* (solar_const) (daytime))
(+ (* (daytime)
(- (* 4 (daytime)) 90)) 450))))
(increase (daytime) (* #t 1)))
)
(:process night_operations
:parameters ()
:precondition (not (day))
:effect (and (increase (daytime) (* #t 1))
(decrease (soc) (* #t (heater_rate))))
)
(:event nightfall
:parameters ()
:precondition (and (day) (>= (daytime) (dusk)))
:effect (and (assign (daytime) (- (dawn)))
(not (day)))
)
(:event daybreak
:parameters ()
:precondition (and (not (day)) (>= (daytime) 0))
:effect (day)
)
(:durative-action fullPrepare
:parameters (?e - equipment)
:duration (= ?duration (fullprepare_durtime))
:condition (and (at start (available ?e))
(over all (> (soc) (safelevel))))
:effect (and (at start (not (available ?e)))
(at start (increase (demand) (a_rate)))
(at end (available ?e))
(at end (decrease (demand) (a_rate)))
(at end (readyForObs1))
(at end (readyForObs2)))
)
(:durative-action prepareObs1
:parameters (?e - equipment)
:duration (= ?duration (prepareobs1_durtime))
:condition (and (at start (available ?e))
(over all (> (soc) (safelevel))))
:effect (and (at start (not (available ?e)))
(at start (increase (demand) (B_rate)))
(at end (available ?e))
(at end (decrease (demand) (B_rate)))
(at end (readyForObs1)))
)
(:durative-action prepareObs2
:parameters (?e - equipment)
:duration (= ?duration (prepareobs2_durtime))
:condition (and (at start (available ?e))
(over all (> (soc) (safelevel))))
:effect (and (at start (not (available ?e)))
(at start (increase (demand) (C_rate)))
(at end (available ?e))
(at end (decrease (demand) (C_rate)))
(at end (readyForObs2)))
)
(:durative-action observe1
:parameters (?e - equipment)
:duration (= ?duration (observe1_durtime))
:condition (and (at start (available ?e))
(at start (readyForObs1))
(over all (> (soc) (safelevel)))
(over all (not (commsOpen))))
:effect (and (at start (not (available ?e)))
(at start (increase (demand) (obs1_rate)))
(at end (available ?e))
(at end (decrease (demand) (obs1_rate)))
(at end (not (readyForObs1)))
(at end (gotObs1)))
)
(:durative-action observe2
:parameters (?e - equipment)
:duration (= ?duration (observe2_durtime))
:condition (and (at start (available ?e))
(at start (readyForObs2))
(over all (> (soc) (safelevel)))
(over all (not (commsOpen))))
:effect (and (at start (not (available ?e)))
(at start (increase (demand) (obs2_rate)))
(at end (available ?e))
(at end (decrease (demand) (obs2_rate)))
(at end (not (readyForObs2)))
(at end (gotObs2)))
)
)


