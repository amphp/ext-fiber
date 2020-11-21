<?php

interface Future
{
    public function __invoke(Fiber $fiber): void;
}
