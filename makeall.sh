#!/bin/bash

# Copyright (c) 2015 University of Waterloo
#
# The contents of this file are covered under the licence agreement in 
# the file "LICENCE" distributed with this repository.

# Make all tested variants of resolver prototype

for DIR in bu co td; do
	# exclude 'top', insufficiently different from 'def'
	for RES in tec def imm; do
		for ENV in per iti bas; do
			# 'td' caching not compatible with 'per' relationship 
			if [ ! \( $DIR = "td" -a $ENV = "per" \) ]; then
				make -j 8 DIR=$DIR RES=$RES ENV=$ENV rp
			fi
		done
	done 
done
