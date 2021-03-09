FROM php:8-cli

COPY ./ /usr/src/php/ext/fiber

RUN cd /usr/src/php/ext/fiber \
	&& docker-php-source extract \
	&& docker-php-ext-install fiber \
	&& make test \
	&& docker-php-source delete
