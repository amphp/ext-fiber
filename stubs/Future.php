<?php

interface Future
{
    /**
     * Schedule the resumption of the given suspending Fiber.
     *
     * @param Fiber $fiber
     */
    public function schedule(\Fiber $fiber): void;
}
