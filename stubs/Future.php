<?php

interface Future
{
    /**
     * Schedule the resumption of the given suspending Fiber.
     *
     * @param Fiber $fiber
     *
     * @return FiberScheduler
     */
    public function schedule(\Fiber $fiber): FiberScheduler;
}
