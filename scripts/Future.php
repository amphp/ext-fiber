<?php

interface Future
{
    public function __invoke(Continuation $continuation): void;
}
