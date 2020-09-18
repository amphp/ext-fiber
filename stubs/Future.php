<?php

interface Future
{
    /**
     * Schedule the resumption of the given suspending Fiber.
     *
     * @param Fiber $fiber
     *
     * @return Scheduler
     */
    public function schedule(\Fiber $fiber): Scheduler;
}
