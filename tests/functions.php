<?php

function get_fibre_status_string($status)
{
    $knownStatuses = [
        Fiber::STATUS_INIT => 'STATUS_INIT',
        Fiber::STATUS_SUSPENDED => 'STATUS_SUSPENDED',
        Fiber::STATUS_RUNNING => 'STATUS_RUNNING',
        Fiber::STATUS_FINISHED => 'STATUS_FINISHED',
        Fiber::STATUS_DEAD => 'STATUS_DEAD',
    ];

    if (array_key_exists($status, $knownStatuses) === false) {
        return "Unknown status [" . $status . "]";
    }

    return $knownStatuses[$status];
}

function assert_fiber_status($expectedStatus, Fiber $f)
{
    if ($f->getStatus() !== $expectedStatus) {
        printf(
            "Expected status %s but fibre has status %s [%s]\n",
            $expectedStatus,
            $f->getStatus(),
            get_fibre_status_string($f->getStatus())
        );
        return;
    }

    printf(
        "Status is correctly %s [%s]\n",
        $f->getStatus(),
        get_fibre_status_string($f->getStatus())
    );
}