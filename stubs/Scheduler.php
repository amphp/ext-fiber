<?php

interface Scheduler
{
    /**
     * Run the scheduler.
     */
    public function run(): void;
}
