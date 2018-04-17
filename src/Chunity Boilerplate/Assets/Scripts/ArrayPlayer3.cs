﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ArrayPlayer3 : MonoBehaviour {

	ChuckSubInstance myChuck;
	Chuck.FloatArrayCallback myFloatArrayCallback;
	Chuck.FloatCallback myFloatCallback;

	public double[] myMidiNotes = { 60, 65, 69, 72 };

	// Use this for initialization
	void Start () {
		myChuck = GetComponent<ChuckSubInstance>();
		myChuck.RunCode( @"
			TriOsc myOsc;
			[60.0] @=> external float myNotes[];
			external Event playMyNotes;
			
			while( true )
			{
				playMyNotes => now;
				myOsc => dac;
				for( 0 => int i; i < myNotes.size(); i++ )
				{
					myNotes[i] => Math.mtof => myOsc.freq;
					100::ms => now;
				}
				<<< myNotes[""numPlayed""], ""played so far"" >>>;
				myOsc =< dac;
			}
		" );

		myFloatArrayCallback = myChuck.CreateGetFloatArrayCallback( GetInitialArrayCallback );
		myFloatCallback = myChuck.CreateGetFloatCallback( GetANumberCallback );
	}
	
	// Update is called once per frame
	private int numPresses = 0;
	void Update () {
		
		if( Input.GetKeyDown( "space" ) )
		{
			// on first press, set entire array
			if( numPresses == 0 )
			{
				myChuck.SetFloatArray( "myNotes", myMidiNotes );
			}
			// on any press, change the value of index 1
			myChuck.SetFloatArrayValue( "myNotes", 1, 60.5f + numPresses );
			// set a dictionary value too
			myChuck.SetAssociativeFloatArrayValue( "myNotes", "numPlayed", numPresses );
			// actually play it!
			myChuck.BroadcastEvent( "playMyNotes" );


			// test some gets too
			myChuck.GetFloatArray( "myNotes", myFloatArrayCallback );
			myChuck.GetFloatArrayValue( "myNotes", 1, myFloatCallback );
			myChuck.GetAssociativeFloatArrayValue( "myNotes", "numPlayed", myFloatCallback );

			numPresses++;
		}
	}

	void GetInitialArrayCallback( double[] values, ulong numValues )
	{
		Debug.Log( "Array started with " + numValues.ToString() + " numbers which were: ");
		for( int i = 0; i < values.Length; i++ )
		{
			Debug.Log( "        " + values[i].ToString() );
		}
	}

	void GetANumberCallback( double value )
	{
		Debug.Log( "I got a number! " + value.ToString() );
	}
}
