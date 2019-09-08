﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;


#if UNITY_WEBGL
using CK_INT = System.Int32;
using CK_UINT = System.UInt32;
using CK_FLOAT = System.Single;
#else
using CK_INT = System.Int64;
using CK_UINT = System.UInt64;
using CK_FLOAT = System.Double;
#endif

public class ChunityExampleGlobalFloatArray : MonoBehaviour
{
	// This example shows how to use various methods
	// for getting and setting global float arrays.

	ChuckSubInstance myChuck;
	Chuck.FloatArrayCallback myFloatArrayCallback;
	Chuck.FloatCallback myFloatCallback;

	public CK_FLOAT[] myMidiNotes = { 60, 65, 69, 72 };

	// Use this for initialization
	void Start()
	{
		myChuck = GetComponent<ChuckSubInstance>();
		myChuck.RunCode( @"
			TriOsc myOsc;
			[60.0] @=> global float myFloatNotes[];
			global Event playMyNotes;
			
			while( true )
			{
				playMyNotes => now;
				myOsc => dac;
				for( 0 => int i; i < myFloatNotes.size(); i++ )
				{
                    <<< ""myFloatNotes["", i, ""] ="", myFloatNotes[i] >>>;
					myFloatNotes[i] => Math.mtof => myOsc.freq;
					100::ms => now;
				}
				<<< myFloatNotes[""numPlayed""], ""played so far"" >>>;
				myOsc =< dac;
			}
		" );

		myFloatArrayCallback = myChuck.CreateGetFloatArrayCallback( GetInitialArrayCallback );
		myFloatCallback = myChuck.CreateGetFloatCallback( GetANumberCallback );
	}

	// Update is called once per frame
	private int numPresses = 0;
	void Update()
	{

		if( Input.GetKeyDown( "space" ) )
		{
			// on first press, set entire array
			if( numPresses == 0 )
			{
				myChuck.SetFloatArray( "myFloatNotes", myMidiNotes );
			}
			// on any press, change the value of index 1
			myChuck.SetFloatArrayValue( "myFloatNotes", 1, 60.5f + numPresses );
			// set a dictionary value too
			myChuck.SetAssociativeFloatArrayValue( "myFloatNotes", "numPlayed", numPresses );


			// test some gets too
			myChuck.GetFloatArray( "myFloatNotes", myFloatArrayCallback );
			myChuck.GetFloatArrayValue( "myFloatNotes", 1, myFloatCallback );
			myChuck.GetAssociativeFloatArrayValue( "myFloatNotes", "numPlayed", myFloatCallback );
			
            // actually play it!
			myChuck.BroadcastEvent( "playMyNotes" );

			numPresses++;
		}
	}

	[AOT.MonoPInvokeCallback(typeof(Chuck.FloatArrayCallback))]
	void GetInitialArrayCallback( CK_FLOAT[] values, CK_UINT numValues )
	{
		Debug.Log( "Array has " + numValues.ToString() + " numbers which are: " );
		for( int i = 0; i < values.Length; i++ )
		{
			Debug.Log( "        " + values[i].ToString() );
		}
	}

	[AOT.MonoPInvokeCallback(typeof(Chuck.FloatCallback))]
	void GetANumberCallback( CK_FLOAT value )
	{
		Debug.Log( "I got a number! " + value.ToString() );
	}
}
