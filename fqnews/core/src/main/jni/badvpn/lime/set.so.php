<?php

/*
File: set.so.php
License: GPL
Purpose: We should really have a "set" data type. It's too useful.
*/

class set {
	function __construct($list=array()) { $this->data = array_count_values($list); }
	function has($item) { return isset($this->data[$item]); }
	function add($item) { $this->data[$item] = true; }
	function del($item) { unset($this->data[$item]); return $item;}
	function all() { return array_keys($this->data); }
	function one() { return key($this->data); }
	function count() { return count($this->data); }
	function pop() { return $this->del($this->one()); }
	function union($that) {
		$progress = false;
		foreach ($that->all() as $item) if (!$this->has($item)) {
			$this->add($item);
			$progress = true;
		}
		return $progress;
	}
	function text() {
		return ' { '.implode(' ', $this->all()).' } ';
	}
}
