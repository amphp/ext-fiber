<?php

class Promise
{
    /** @var Fiber[] */
    private array $fibers = [];
    private Scheduler $scheduler;
    private bool $resolved = false;
    private ?Throwable $error = null;
    private mixed $result;

    public function __construct(Scheduler $scheduler)
    {
        $this->scheduler = $scheduler;
    }

    public function await(): mixed
    {
        if (!$this->resolved) {
            $this->schedule(Fiber::this());
            return Fiber::suspend($this->scheduler->getSchedulerFiber());
        }

        if ($this->error) {
            throw $this->error;
        }

        return $this->result;
    }

    public function schedule(Fiber $fiber): void
    {
        if ($this->resolved) {
            if ($this->error !== null) {
                $this->scheduler->defer(fn() => $fiber->throw($this->error));
            } else {
                $this->scheduler->defer(fn() => $fiber->resume($this->result));
            }

            return;
        }

        $this->fibers[] = $fiber;
    }

    public function resolve(mixed $value = null): void
    {
        if ($this->resolved) {
            throw new Error("Promise already resolved");
        }

        $this->result = $value;
        $this->continue();
    }

    public function fail(Throwable $error): void
    {
        if ($this->resolved) {
            throw new Error("Promise already resolved");
        }

        $this->error = $error;
        $this->continue();
    }

    private function continue(): void
    {
        $this->resolved = true;

        $fibers = $this->fibers;
        $this->fibers = [];

        foreach ($fibers as $fiber) {
            $this->schedule($fiber);
        }
    }
}

class Scheduler
{
    private SchedulerFiber $scheduler;
    /** @var resource */
    private $curl;
    /** @var callable[] */
    private array $defers = [];
    /** @var Fiber[] */
    private array $fibers = [];

    public function __construct()
    {
        $this->curl = curl_multi_init();
    }

    public function __destruct()
    {
        curl_multi_close($this->curl);
    }

    public function getSchedulerFiber(): SchedulerFiber
    {
        if (!isset($this->scheduler) || $this->scheduler->isTerminated()) {
            $this->scheduler = new SchedulerFiber(fn() => $this->run());
        }

        return $this->scheduler;
    }

    public function fetch(string $url): string
    {
        $curl = curl_init();

        curl_setopt($curl, CURLOPT_URL, $url);
        curl_setopt($curl, CURLOPT_HEADER, 0);
        curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);

        curl_multi_add_handle($this->curl, $curl);

        $this->fibers[(int) $curl] = Fiber::this();
        Fiber::suspend($this->getSchedulerFiber());

        $status = curl_getinfo($curl, CURLINFO_RESPONSE_CODE);
        if ($status !== 200) {
            throw new Exception(sprintf('Request to %s failed with status code %d', $url, $status));
        }

        $body = substr(trim(curl_multi_getcontent($curl)), 0, 255);

        curl_close($curl);

        return $body;
    }

    public function defer(callable $callable): void
    {
        $this->defers[] = $callable;
    }

    public function async(callable $callable): Promise
    {
        $promise = new Promise($this);

        $fiber = new Fiber(function () use ($promise, $callable) {
            try {
                $promise->resolve($callable());
            } catch (Throwable $e) {
                $promise->fail($e);
            }
        });

        $this->defer(fn() => $fiber->start());

        return $promise;
    }

    public function run(): void
    {
        do {
            do {
                $defers = $this->defers;
                $this->defers = [];

                foreach ($defers as $callable) {
                    $callable();
                }

                $status = curl_multi_exec($this->curl, $active);
                if ($active) {
                    $select = curl_multi_select($this->curl);
                    if ($select > 0) {
                        $this->processQueue();
                    }
                }
            } while ($active && $status === CURLM_OK);

            $this->processQueue();
        } while ($this->defers);
    }

    private function processQueue(): void
    {
        while ($info = curl_multi_info_read($this->curl)) {
            if ($info['msg'] !== CURLMSG_DONE) {
                continue;
            }

            $fiber = $this->fibers[(int) $info['handle']];
            $fiber->resume();
        }
    }
}

function await(Promise ...$promises): array
{
    return array_map(fn($promise) => $promise->await(), $promises);
}

$urls = array_fill(0, $argv[1] ?? 10, 'https://amphp.org/');

$scheduler = new Scheduler;

$promises = [];
foreach ($urls as $url) {
    $promises[] = $scheduler->async(fn() => $scheduler->fetch($url));
}

print 'Starting to make ' . count($promises) . ' requests...' . PHP_EOL;

$start = hrtime(true);

$responses = await(...$promises);

// var_dump($responses);

print ((hrtime(true) - $start) / 1_000_000) . 'ms' . PHP_EOL;
