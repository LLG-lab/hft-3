<?php

class Logger
{
    const NOTICE  = 0;
    const WARNING = 1;
    const ERROR   = 2;
    const FATAL   = 3;
    private $pid;

    public function __construct() {
        $this -> pid = getmypid();
    }

    public function destruct() {
        // Nothing to do.
    }

    public function write($severity, $message) {
        $label = self::get_time_label($this -> pid);

        switch ($severity) {
            case self::NOTICE:
                $label .= " ";
                break;
            case self::WARNING:
                $label .= " [WARNING] ";
                break;
            case self::ERROR:
                $label .= " [ERROR!] ";
                break;
            case self::FATAL:
                $label .= " [*** FATAL ERROR ***] ";
                break;
        }

        echo $label.$message.chr(10);
    }

    private static function get_time_label($pid) {
        return '[' . date("D M j G:i:s T Y") . '] (pid '.$pid.')';
    }

}