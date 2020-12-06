<?php

interface Future
{
    public function schedule(Fiber $fiber): void;
}
