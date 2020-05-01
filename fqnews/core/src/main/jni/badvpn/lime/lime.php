<?php
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
define('LIME_DIR', dirname(__FILE__));

function emit($str) { fputs(STDERR, $str."\n"); }

class Bug extends Exception {}
function bug($gripe='Bug found.') { throw new Bug($gripe); }
function bug_if($falacy, $gripe='Bug found.') { if ($falacy) throw new Bug($gripe); }
function bug_unless($assertion, $gripe='Bug found.') { if (!$assertion) throw new Bug($gripe); }

include_once(LIME_DIR.'/parse_engine.php');
include_once(LIME_DIR.'/set.so.php');
include_once(LIME_DIR.'/flex_token_stream.php');

function lime_token_reference($pos) { return '$tokens['.$pos.']'; }
function lime_token_reference_callback($foo) { return lime_token_reference($foo[1]-1); }

class cf_action {
	function __construct($code) { $this->code=$code; }
}
class step {
	/*
	Base class for parse table instructions. The main idea is to make the
	subclasses responsible for conflict resolution among themselves. It also
	forms a sort of interface to the parse table.
	*/
	function __construct($sym) {
		bug_unless($sym instanceof sym);
		$this->sym = $sym;
	}
	function glyph() { return $this->sym->name; }
}
class error extends step {
	function sane() { return false; }
	function instruction() { bug("This should not happen."); }
	function decide($that) { return $this; /* An error shall remain one. */ }
}
class shift extends step {
	function __construct($sym, $q) {
		parent::__construct($sym);
		$this->q = $q;
	}
	function sane() { return true; }
	function instruction() { return "s $this->q"; }
	function decide($that) {
		# shift-shift conflicts are impossible.
		# shift-accept conflicts are a bug.
		# so we can infer:
		bug_unless($that instanceof reduce);
		
		# That being said, the resolution is a matter of precedence.
		$shift_prec = $this->sym->right_prec;
		$reduce_prec = $that->rule->prec;
		
		# If we don't have defined precedence levels for both options,
		# then we default to shifting:
		if (!($shift_prec and $reduce_prec)) return $this;
		
		# Otherwise, use the step with higher precedence.
		if ($shift_prec > $reduce_prec) return $this;
		if ($reduce_prec > $shift_prec) return $that;
		
		# The "nonassoc" works by giving equal precedence to both options,
		# which means to put an error instruction in the parse table.
		return new error($this->sym);
	}
}
class reduce extends step {
	function __construct($sym, $rule) {
		bug_unless($rule instanceof rule);
		parent::__construct($sym);
		$this->rule = $rule;
	}
	function sane() { return true; }
	function instruction() { return 'r '.$this->rule->id; }
	function decide($that) {
		# This means that the input grammar has a reduce-reduce conflict.
		# Such things are considered an error in the input.
		throw new RRC($this, $that);
		#exit(1);
		# BISON would go with the first encountered reduce thus:
		# return $this;
	}
}
class accept extends step {
	function __construct($sym) { parent::__construct($sym); }
	function sane() { return true; }
	function instruction() { return 'a '.$this->sym->name; }
}
class RRC extends Exception {
	function __construct($a, $b) {
		parent::__construct("Reduce-Reduce Conflict");
		$this->a = $a;
		$this->b = $b;
	}
	function make_noise() {
		emit(sprintf(
			"Reduce-Reduce Conflict:\n%s\n%s\nLookahead is (%s)",
			$this->a->rule->text(),
			$this->b->rule->text(),
			$this->a->glyph()
		));
	}
}
class state {
	function __construct($id, $key, $close) {
		$this->id = $id;
		$this->key = $key;
		$this->close = $close;	# config key -> object
		ksort($this->close);
		$this->action = array();
	}
	function dump() {
		echo " * ".$this->id.' / '.$this->key."\n";
		foreach ($this->close as $config) $config->dump();
	}
	function add_shift($sym, $state) {
		$this->add_instruction(new shift($sym, $state->id));
	}
	function add_reduce($sym, $rule) {
		$this->add_instruction(new reduce($sym, $rule));
	}
	function add_accept($sym) {
		$this->add_instruction(new accept($sym));
	}
	function add_instruction($step) {
		bug_unless($step instanceof step);
		$this->action[] = $step;
	}
	function find_reductions($lime) {
		# rightmost configurations followset yields reduce.
		foreach($this->close as $c) {
			if ($c->rightmost) {
				foreach ($c->follow->all() as $glyph) $this->add_reduce($lime->sym($glyph), $c->rule);
			}
		}
	}
	function resolve_conflicts() {
		# For each possible lookahead, find one (and only one) step to take.
		$table = array();
		foreach ($this->action as $step) {
			$glyph = $step->glyph();
			if (isset($table[$glyph])) {
				# There's a conflict. The shifts all came first, which
				# simplifies the coding for the step->decide() methods.
				try {
					$table[$glyph] = $table[$glyph]->decide($step);
				} catch (RRC $e) {
					emit("State $this->id:");
					$e->make_noise();
				}
			} else {
				# This glyph is yet unprocessed, so the step at hand is
				# our best current guess at what the grammar indicates.
				$table[$glyph] = $step;
			}
		}
		
		# Now that we have the correct steps chosen, this routine is oddly
		# also responsible for turning that table into the form that will
		# eventually be passed to the parse engine. (So FIXME?)
		$out = array();
		foreach ($table as $glyph => $step) {
			if ($step->sane()) $out[$glyph] = $step->instruction();
		}
		return $out;
	}
	function segment_config() {
		# Filter $this->close into categories based on the symbol_after_the_dot.
		$f = array();
		foreach ($this->close as $c) {
			$p = $c->symbol_after_the_dot;
			if (!$p) continue;
			$f[$p->name][] = $c;
		}
		return $f;
	}
}
class sym {
	function __construct($name, $id) {
		$this->name=$name;
		$this->id=$id;
		$this->term = true;	# Until proven otherwise.
		$this->rule = array();
		$this->config = array();
		$this->lambda = false;
		$this->first = new set();
		$this->left_prec = $this->right_prec = 0;
	}
	function summary() {
		$out = '';
		foreach ($this->rule as $rule) $out .= $rule->text()."\n";
		return $out;
	}
}
class rule {
	function __construct($id, $sym, $rhs, $code, $look, $replace) {
		$this->id = $id;
		$this->sym = $sym;
		$this->rhs = $rhs;
		$this->code = $code;
		$this->look = $look;
		bug_unless(is_int($look));
		$this->replace = $replace;
		#$this->prec_sym = $prec_sym;
		$this->prec = 0;
		$this->first = array();
		$this->epsilon = count($rhs);
	}
	function lhs_glyph() { return $this->sym->name; }
	function determine_precedence() {
		# We may eventually expand to allow explicit prec_symbol declarations.
		# Until then, we'll go with the rightmost terminal, which is what
		# BISON does. People probably expect that. The leftmost terminal
		# is a reasonable alternative behaviour, but I don't see the big
		# deal just now.
		
		#$prec_sym = $this->prec_sym;
		#if (!$prec_sym)
		$prec_sym = $this->rightmost_terminal();
		if (!$prec_sym) return;
		$this->prec = $prec_sym->left_prec;
	}
	private function rightmost_terminal() {
		$symbol = NULL;
		$rhs = $this->rhs;
		while ($rhs) {
			$symbol = array_pop($rhs);
			if ($symbol->term) break;
		}
		return $symbol;
	}
	function text() {
		$t = "($this->id) ".$this->lhs_glyph().' :=';
		foreach($this->rhs as $s) $t .= '  '.$s->name;
		return $t;
	}
	function table(lime_language $lang) {
		return array(
			'symbol' => $this->lhs_glyph(),
			'len' => $this->look,
			'replace' => $this->replace,
			'code' => $lang->fixup($this->code),
			'text' => $this->text(),
		);
	}
	function lambda() {
		foreach ($this->rhs as $sym) if (!$sym->lambda) return false;
		return true;
	}
	function find_first() {
		$dot = count($this->rhs);
		$last  = $this->first[$dot] = new set();
		while ($dot) {
			$dot--;
			$symbol_after_the_dot = $this->rhs[$dot];
			$first = $symbol_after_the_dot->first->all();
			bug_if(empty($first) and !$symbol_after_the_dot->lambda);
			$set = new set($first);
			if ($symbol_after_the_dot->lambda) {
				$set->union($last);
				if ($this->epsilon == $dot+1) $this->epsilon = $dot;
			}
			$last = $this->first[$dot] = $set;
		}
	}
	function teach_symbol_of_first_set() {
		$go = false;
		foreach ($this->rhs as $sym) {
			if ($this->sym->first->union($sym->first)) $go = true;
			if (!$sym->lambda) break;
		}
		return $go;
	}
	function lambda_from($dot) {
		return $this->epsilon <= $dot;
	}
	function leftmost($follow) {
		return new config($this, 0, $follow);
	}
	function dotted_text($dot) {
		$out = $this->lhs_glyph().' :=';
		$idx = -1;
		foreach($this->rhs as $idx => $s) {
			if ($idx == $dot) $out .= ' .';
			$out .= '  '.$s->name;
		}
		if ($dot > $idx) $out .= ' .';
		return $out;
	}
}
class config {
	function __construct($rule, $dot, $follow) {
		$this->rule=$rule;
		$this->dot = $dot;
		$this->key = "$rule->id.$dot";
		$this->rightmost = count($rule->rhs) <= $dot;
		$this->symbol_after_the_dot = $this->rightmost ? null : $rule->rhs[$dot];
		$this->_blink = array();
		$this->follow = new set($follow);
		$this->_flink=  array();
		bug_unless($this->rightmost or count($rule));
	}
	function text() {
		$out = $this->rule->dotted_text($this->dot);
		$out .= ' [ '.implode(' ', $this->follow->all()).' ]';
		return $out;
	}
	function blink($config) {
		$this->_blink[] = $config;
	}
	function next() {
		bug_if($this->rightmost);
		$c = new config($this->rule, $this->dot+1, array());
		# Anything in the follow set for this config will also be in the next.
		# However, we link it backwards because we might wind up selecting a
		# pre-existing state, and the housekeeping is easier in the first half
		# of the program. We'll fix it before doing the propagation.
		$c->blink($this);
		return $c;
	}
	function copy_links_from($that) {
		foreach($that->_blink as $c) $this->blink($c);
	}
	function lambda() {
		return $this->rule->lambda_from($this->dot);
	}
	function simple_follow() {
		return $this->rule->first[$this->dot+1]->all();
	}
	function epsilon_follows() {
		return $this->rule->lambda_from($this->dot+1);
	}
	function fixlinks() {
		foreach ($this->_blink as $that) $that->_flink[] = $this;
		$this->blink = array();
	}
	function dump() {
		echo "   * ";
		echo $this->key.' : ';
		echo $this->rule->dotted_text($this->dot);
		echo $this->follow->text();
		foreach ($this->_flink as $c) echo $c->key.' / ';
		echo "\n";
	}
}
class lime {
	var $parser_class = 'parser';
	function __construct() {
		$this->p_next = 1;
		$this->sym = array();
		$this->rule = array();
		$this->start_symbol_set = array();
		$this->state = array();
		$this->stop = $this->sym('#');
		#$err = $this->sym('error');
		$err->term = false;
		$this->lang = new lime_language_php();
	}
	function language() { return $this->lang; }
	function build_parser() {
		$this->add_start_rule();
		foreach ($this->rule as $r) $r->determine_precedence();
		$this->find_sym_lamdba();
		$this->find_sym_first();
		foreach ($this->rule as $rule) $rule->find_first();
		$initial = $this->find_states();
		$this->fixlinks();
		# $this->dump_configurations();
		$this->find_follow_sets();
		foreach($this->state as $s) $s->find_reductions($this);
		$i = $this->resolve_conflicts();
		$a = $this->rule_table();
		$qi = $initial->id;
		return $this->lang->ptab_to_class($this->parser_class, compact('a', 'qi', 'i'));
	}
	function rule_table() {
		$s = array();
		foreach ($this->rule as $i => $r) {
			$s[$i] = $r->table($this->lang);
		}
		return $s;
	}
	function add_rule($symbol, $rhs, $code) {
		$this->add_raw_rule($symbol, $rhs, $code, count($rhs), true);
	}
	function trump_up_bogus_lhs($real) {
		return "'$real'".count($this->rule);
	}
	function add_raw_rule($lhs, $rhs, $code, $look, $replace) { 
		$sym = $this->sym($lhs);
		$sym->term=false;
		if (empty($rhs)) $sym->lambda = true;
		$rs = array();
		foreach ($rhs as $str) $rs[] = $this->sym($str);
		$rid = count($this->rule);
		$r = new rule($rid, $sym, $rs, $code, $look, $replace);
		$this->rule[$rid] = $r;
		$sym->rule[] = $r;
	}
	function sym($str) {
		if (!isset($this->sym[$str])) $this->sym[$str] = new sym($str, count($this->sym));
		return $this->sym[$str];
	}
	function summary() {
		$out = '';
		foreach ($this->sym as $sym) if (!$sym->term) $out .= $sym->summary();
		return $out;
	}
	private function find_sym_lamdba() {
		do {
			$go = false;
			foreach ($this->sym as $sym) if (!$sym->lambda) {
				foreach ($sym->rule as $rule) if ($rule->lambda()) {
					$go = true;
					$sym->lambda = true;
				}
			}
		} while ($go);
	}
	private function teach_terminals_first_set() {
		foreach ($this->sym as $sym) if ($sym->term) $sym->first->add($sym->name);
	}
	private function find_sym_first() {
		$this->teach_terminals_first_set();
		do {
			$go = false;
			foreach ($this->rule as $r) if ($r->teach_symbol_of_first_set()) $go = true;
		} while ($go);
	}
	function add_start_rule() {
		$rewrite = new lime_rewrite("'start'");
		$rhs = new lime_rhs();
		$rhs->add(new lime_glyph($this->deduce_start_symbol()->name, NULL));
		#$rhs->add(new lime_glyph($this->stop->name, NULL));
		$rewrite->add_rhs($rhs);
		$rewrite->update($this);
	}
	private function deduce_start_symbol() {
		$candidate = current($this->start_symbol_set);
		# Did the person try to set a start symbol at all?
		if (!$candidate) return $this->first_rule_lhs();
		# Do we actually have such a symbol on the left of a rule?
		if ($candidate->terminal) return $this->first_rule_lhs();
		# Ok, it's a decent choice. We need to return the symbol entry.
		return $this->sym($candidate);
	}
	private function first_rule_lhs() {
		reset($this->rule);
		$r = current($this->rule);
		return $r->sym;
	}
	function find_states() {
		/*
		Build an initial state. This is a recursive process which digs out
		the LR(0) state graph. 
		*/
		$start_glyph = "'start'";
		$sym = $this->sym($start_glyph);
		$basis = array();
		foreach($sym->rule as $rule) {
			$c = $rule->leftmost(array('#'));
			$basis[$c->key] = $c;
		}
		$initial = $this->get_state($basis);
		$initial->add_accept($sym);
		return $initial;
	}
	function get_state($basis) {
		$key = array_keys($basis);
		sort($key);
		$key = implode(' ', $key);
		if (isset($this->state[$key])) {
			# Copy all the links around...
			$state = $this->state[$key];
			foreach($basis as $config) $state->close[$config->key]->copy_links_from($config);
			return $state;
		} else {
			$close = $this->state_closure($basis);
			$this->state[$key] = $state = new state(count($this->state), $key, $close);
			$this->build_shifts($state);
			return $state;
		}
	}
	private function state_closure($q) {
		# $q is a list of config.
		$close = array();
		while ($config = array_pop($q)) {
			if (isset($close[$config->key])) {
				$close[$config->key]->copy_links_from($config);
				$close[$config->key]->follow->union($config->follow);
				continue;
			}
			$close[$config->key] = $config;
			
			$symbol_after_the_dot = $config->symbol_after_the_dot;
			if (!$symbol_after_the_dot) continue;
			
			if (! $symbol_after_the_dot->term) {
				foreach ($symbol_after_the_dot->rule as $r) {
					$station = $r->leftmost($config->simple_follow());
					if ($config->epsilon_follows()) $station->blink($config);
					$q[] = $station;
				}
				# The following turned out to be wrong. Don't do it.
				#if ($symbol_after_the_dot->lambda) {
				#	$q[] = $config->next();
				#}
			}
			
		}
		return $close;
	}
	function build_shifts($state) {
		foreach ($state->segment_config() as $glyph => $segment) {
			$basis = array();
			foreach ($segment as $preshift) {
				$postshift = $preshift->next();
				$basis[$postshift->key] = $postshift;
			}
			$dest = $this->get_state($basis);
			$state->add_shift($this->sym($glyph), $dest);
		}
	}
	function fixlinks() {
		foreach ($this->state as $s) foreach ($s->close as $c) $c->fixlinks();
	}
	function find_follow_sets() {
		$q = array();
		foreach ($this->state as $s) foreach ($s->close as $c) $q[] = $c;
		while ($q) {
			$c = array_shift($q);
			foreach ($c->_flink as $d) {
				if ($d->follow->union($c->follow)) $q[] = $d;
			}
		}
	}
	private function set_assoc($ss, $l, $r) {
		$p = ($this->p_next++)*2;
		foreach ($ss as $glyph) {
			$s = $this->sym($glyph);
			$s->left_prec = $p+$l;
			$s->right_prec = $p+$r;
		}
	}
	function left_assoc($ss) { $this->set_assoc($ss, 1, 0); }
	function right_assoc($ss) { $this->set_assoc($ss, 0, 1); }
	function non_assoc($ss) { $this->set_assoc($ss, 0, 0); }
	private function resolve_conflicts() {
		# For each state, try to find one and only one
		# thing to do for any given lookahead.
		$i = array();
		foreach ($this->state as $s) $i[$s->id] = $s->resolve_conflicts();
		return $i;
	}
	function dump_configurations() {
		foreach ($this->state as $q) $q->dump();
	}
	function dump_first_sets() {
		foreach ($this->sym as $s) {
			echo " * ";
			echo $s->name.' : ';
			echo $s->first->text();
			echo "\n";
		}
	}
	function add_rule_with_actions($lhs, $rhs) {
		# First, make sure this thing is well-formed.
		if(!is_object(end($rhs))) $rhs[] = new cf_action('');
		# Now, split it into chunks based on the actions.
		$look = -1;
		$subrule = array();
		$subsymbol = '';
		while (count($rhs)) {
			$it = array_shift($rhs);
			$look ++;
			if (is_string($it)) {
				$subrule[] = $it;
			} else {
				$code = $it->code;
				# It's an action.
				# Is it the last one?
				if (count($rhs)) {
					# no.
					$subsymbol = $this->trump_up_bogus_lhs($lhs);
					$this->add_raw_rule($subsymbol, $subrule, $code, $look, false);
					$subrule = array($subsymbol);
				} else {
					# yes.
					$this->add_raw_rule($lhs, $subrule, $code, $look, true);
				}
			}
		}
	}
	function pragma($type, $args) {
		switch ($type) {
			case 'left':
			$this->left_assoc($args);
			break;
			
			case 'right':
			$this->right_assoc($args);
			break;
			
			case 'nonassoc':
			$this->non_assoc($args);
			break;
			
			case 'start':
			$this->start_symbol_set = $args;
			break;
			
			case 'class':
			$this->parser_class = $args[0];
			break;
			
			default:
			emit(sprintf("Bad Parser Pragma: (%s)", $type));
			exit(1);
		}
	}
}
class lime_language {}
class lime_language_php extends lime_language {
	private function result_code($expr) { return "\$result = $expr;\n"; }
	function default_result() { return $this->result_code('reset($tokens)'); }
	function result_pos($pos) { return $this->result_code(lime_token_reference($pos)); }
	function bind($name, $pos) { return "\$$name =& \$tokens[$pos];\n"; }
	function fixup($code) {
		$code = preg_replace_callback('/\\$(\d+)/', 'lime_token_reference_callback', $code);
		$code = preg_replace('/\\$\\$/', '$result', $code);
		return $code;
	}
	function to_php($code) {
		return $code;
	}
	function ptab_to_class($parser_class, $ptab) {
		$code = "class $parser_class extends lime_parser {\n";
		$code .= 'var $qi = '.var_export($ptab['qi'], true).";\n";
		$code .= 'var $i = '.var_export($ptab['i'], true).";\n";
		
		
		$rc = array();
		$method = array();
		$rules = array();
		foreach($ptab['a'] as $k => $a) {
			$symbol = preg_replace('/[^\w]/', '', $a['symbol']);
			$rn = ++$rc[$symbol];
			$mn = "reduce_${k}_${symbol}_${rn}";
			$method[$k] = $mn;
			$comment = "#\n# $a[text]\n#\n";
			$php = $this->to_php($a['code']);
			$code .= "function $mn(".LIME_CALL_PROTOCOL.") {\n$comment$php\n}\n\n";
			
			
			unset($a['code']);
			unset($a['text']);
			$rules[$k] = $a;
		}
		
		$code .= 'var $method = '.var_export($method, true).";\n";
		$code .= 'var $a = '.var_export($rules, true).";\n";
		
		
		
		$code .= "}\n";
		#echo $code;
		return $code;
	}
}
class lime_rhs {
	function __construct() {
		/**
		Construct and add glyphs and actions in whatever order.
		Then, add this to a lime_rewrite.
		
		Don't call install_rule.
		The rewrite will do that for you when you "update" with it.
		*/
		$this->rhs = array();
	}
	function add($slot) {
		bug_unless($slot instanceof lime_slot);
		$this->rhs[] = $slot;
	}
	function install_rule(lime $lime, $lhs) {
		# This is the part that has to break the rule into subrules if necessary.
		$rhs = $this->rhs;
		# First, make sure this thing is well-formed.
		if (!(end($rhs) instanceof lime_action)) $rhs[] = new lime_action('', NULL);
		# Now, split it into chunks based on the actions.
		
		$lang = $lime->language();
		$result_code = $lang->default_result();
		$look = -1;
		$subrule = array();
		$subsymbol = '';
		$preamble = '';
		while (count($rhs)) {
			$it = array_shift($rhs);
			$look ++;
			if ($it instanceof lime_glyph) {
				$subrule[] = $it->data;
			} elseif ($it instanceof lime_action) {
				$code = $it->data;
				# It's an action.
				# Is it the last one?
				if (count($rhs)) {
					# no.
					$subsymbol = $lime->trump_up_bogus_lhs($lhs);
					$action = $lang->default_result().$preamble.$code;
					$lime->add_raw_rule($subsymbol, $subrule, $action, $look, false);
					$subrule = array($subsymbol);
				} else {
					# yes.
					$action = $result_code.$preamble.$code;
					$lime->add_raw_rule($lhs, $subrule, $action, $look, true);
				}
			} else {
				impossible();
			}
			if ($it->name == '$') $result_code = $lang->result_pos($look);
			elseif ($it->name) $preamble .= $lang->bind($it->name, $look);
		}
	}
}
class lime_rewrite {
	function __construct($glyph) {
		/**
		Construct one of these with the name of the lhs.
		Add some rhs-es to it.
		Finally, "update" the lime you're building.
		*/
		$this->glyph = $glyph;
		$this->rhs = array();
	}
	function add_rhs($rhs) {
		bug_unless($rhs instanceof lime_rhs);
		$this->rhs[] = $rhs;
	}
	function update(lime $lime) {
		foreach ($this->rhs as $rhs) {
			$rhs->install_rule($lime, $this->glyph);
			
		}
	}
}
class lime_slot {
	/**
	This keeps track of one position in an rhs.
	We specialize to handle actions and glyphs.
	If there is a name for the slot, we store it here.
	Later on, this structure will be consulted in the formation of
	actual production rules.
	*/
	function __construct($data, $name) {
		$this->data = $data;
		$this->name = $name;
	}
	function preamble($pos) {
		if (strlen($this->name) > 0) {
			return "\$$this->name =& \$tokens[$pos];\n";
		}
	}
}
class lime_glyph extends lime_slot {}
class lime_action extends lime_slot {}
function lime_bootstrap() {
	
	/*
	
	This function isn't too terribly interesting to the casual observer.
	You're probably better off looking at parse_lime_grammar() instead.
	
	Ok, if you insist, I'll explain.
	
	The input to Lime is a CFG parser definition. That definition is
	written in some language. (The Lime language, to be exact.)
	Anyway, I have to parse the Lime language and compile it into a
	very complex data structure from which a parser is eventually
	built. What better way than to use Lime itself to parse its own
	language? Well, it's almost that simple, but not quite.
	
	The Lime language is fairly potent, but a restricted subset of
	its features was used to write a metagrammar. Then, I hand-translated
	that metagrammar into another form which is easy to snarf up.
	In the process of reading that simplified form, this function 
	builds the same sort of data structure that later gets turned into
	a parser. The last step is to run the parser generation algorithm,
	eval() the resulting PHP code, and voila! With no hard work, I can
	suddenly read and comprehend the full range of the Lime language
	without ever having written an algorithm to do so. It feels like magic.
	
	*/
	
	$bootstrap = LIME_DIR."/lime.bootstrap";
	$lime = new lime();
	$lime->parser_class = 'lime_metaparser';
	$rhs = array();
	bug_unless(is_readable($bootstrap));
	foreach(file($bootstrap) as $l) {
		$a = explode(":", $l, 2);
		if (count($a) == 2) {
			list($pattern, $code) = $a;
			$sl = new lime_rhs();
			$pattern = trim($pattern);
			if (strlen($pattern)>0) {
				foreach (explode(' ', $pattern) as $glyph) $sl->add(new lime_glyph($glyph, NULL));
			}
			$sl->add(new lime_action($code, NULL));
			$rhs[] = $sl;
		} else {
			$m = preg_match('/^to (\w+)$/', $l, $r);
			if ($m == 0) continue;
			$g = $r[1];
			$rw = new lime_rewrite($g);
			foreach($rhs as $b) $rw->add_rhs($b);
			$rw->update($lime);
			$rhs = array();
		}
	}
	$parser_code = $lime->build_parser();
	eval($parser_code);
}

class voodoo_scanner extends flex_scanner {
	/*
	
	The voodoo is in the way I do lexical processing on grammar definition
	files. They contain embedded bits of PHP, and it's important to keep
	track of things like strings, comments, and matched braces. It seemed
	like an ideal problem to solve with GNU flex, so I wrote a little
	scanner in flex and C to dig out the tokens for me. Of course, I need
	the tokens in PHP, so I designed a simple binary wrapper for them which
	also contains line-number information, guaranteed to help out if you
	write a grammar which surprises the parser in any manner.
	
	*/
	function executable() { return LIME_DIR.'/lime_scan_tokens'; }
}

function parse_lime_grammar($path) {
	/*
	
	This is a good function to read because it teaches you how to interface
	with a Lime parser. I've tried to isolate out the bits that aren't
	instructive in that regard.
	
	*/
	if (!class_exists('lime_metaparser')) lime_bootstrap();
	
	$parse_engine = new parse_engine(new lime_metaparser());
	$scanner = new voodoo_scanner($path);
	try {
		# The result of parsing a Lime grammar is a Lime object.
		$lime = $scanner->feed($parse_engine);
		# Calling its build_parser() method gets the output PHP code.
		return $lime->build_parser();
	} catch (parse_error $e) {
		die ($e->getMessage()." in $path line $scanner->lineno.\n");
	}
}


if ($_SERVER['argv']) {
	$code = '';
	array_shift($_SERVER['argv']);	# Strip out the program name.
	foreach ($_SERVER['argv'] as $path) {
		$code .= parse_lime_grammar($path);
	}
	
	echo "<?php\n\n";
?>

/*

DON'T EDIT THIS FILE!

This file was automatically generated by the Lime parser generator.
The real source code you should be looking at is in one or more
grammar files in the Lime format.

THE ONLY REASON TO LOOK AT THIS FILE is to see where in the grammar
file that your error happened, because there are enough comments to
help you debug your grammar.

If you ignore this warning, you're shooting yourself in the brain,
not the foot.

*/

<?php
	echo $code;
}
